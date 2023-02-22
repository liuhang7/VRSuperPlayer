#include "codec_instance.h"

#pragma comment(lib,"avcodec.lib")
#pragma comment(lib,"avformat.lib")
#pragma comment(lib,"swscale.lib")
#pragma comment(lib,"avutil.lib")

int codec_instance::open_file()
{
	if (b_running)
	{
		close_file();
	}
	av_register_all();//注册解码器
	fc = NULL;
	int res = avformat_open_input(&fc, filepath.c_str(), NULL, NULL);//打开文件
	if (res < 0) {
		printf("error %x in avformat_open_input\n", res);
		return -1;
	}

	res = avformat_find_stream_info(fc, NULL);//取出流信息
	if (res < 0)
	{
		printf("error %x in avformat_find_stream_info\n", res);
		return -1;
	}
	//查找视频流和音频流
	av_dump_format(fc, 0, filepath.c_str(), 0);//列出输入文件的相关流信息

	for (int i = 0; i < fc->nb_streams; i++)
	{
		if (fc->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			videoindex = i;
			break;
		}
	}
	if (videoindex == -1)
	{
		av_log(NULL, AV_LOG_DEBUG, "can't find video stream\n");
		return -1;

	}//没有找到视频流 

	codec = avcodec_find_decoder(fc->streams[videoindex]->codecpar->codec_id);	//根据流信息找到解码器
	if (!codec)
	{
		printf("decoder not found\n");
		return -1;
	}

	codecctx = avcodec_alloc_context3(codec);
	avcodec_parameters_to_context(codecctx, fc->streams[videoindex]->codecpar);
	codecctx->thread_count = 0;
	res = avcodec_open2(codecctx, codec, NULL);
	if (res < 0) {
		printf("error %x in avcodec_open2\n", res);
		return -1;
	}
	b_running = true;
	file_thrd.m_thread = std::thread(std::mem_fn(&codec_instance::file_read_proc), this);
	decode_thrd.m_thread = std::thread(std::mem_fn(&codec_instance::video_decode_proc), this);
	render_thrd.m_thread = std::thread(std::mem_fn(&codec_instance::video_render_proc), this);
	return 0;
}

int codec_instance::close_file()
{
	b_running = false;
	printf("####start exit####\n");
	if (file_thrd.m_thread.joinable())
	{
		file_thrd.m_thread.join();
	}
	if (decode_thrd.m_thread.joinable())
	{
		decode_thrd.m_thread.join();
	}
	if (render_thrd.m_thread.joinable())
	{
		render_thrd.m_thread.join();
	}
	if (render_ins)
	{
		render_ins->deinit_video_render();
		delete render_ins;
	}
	printf("####end exit####\n");
	if (codecctx != NULL)
	{
		avcodec_close(codecctx);
		codecctx = NULL;
	}
	if (fc != NULL)
	{
		avformat_close_input(&fc);
		fc = NULL;
	}
	read_pool.clear_packet();
	return 0;
}

void codec_instance::file_read_proc(void)
{
	std::unique_lock<std::mutex> lck(file_thrd.mtx);
	while (b_running)
	{
		if (b_pausing)
		{
			file_thrd.cond.wait_for(lck, std::chrono::microseconds(40));
			continue;
		}

		AVPacket *packet = av_packet_alloc();

		if ((av_read_frame(fc, packet) == 0))
		{
			if (packet->stream_index == videoindex)
			{
				int ret = -1;
				do {
					ret = read_pool.push_packet(packet);
					if (ret != 0)
					{
						file_thrd.cond.wait_for(lck, std::chrono::microseconds(40));
					}
					else
					{
						decode_thrd.cond.notify_all();
					}
				} while (b_running && !b_pausing && ret != 0);
			}
			else
			{
				av_packet_unref(packet);
			}
		}
		else
		{
			av_seek_frame(fc, videoindex,0,0);
		}
	}
	printf("fileread thread exit\n");
}

void codec_instance::video_decode_proc(void)
{
	std::unique_lock<std::mutex> lck(decode_thrd.mtx);
	while (b_running)
	{
		if (b_pausing)
		{
			decode_thrd.cond.wait_for(lck, std::chrono::microseconds(40));
			continue;
		}
		if (read_pool.peek_packet() != 0)
		{
			file_thrd.cond.notify_all();
			decode_thrd.cond.wait_for(lck, std::chrono::microseconds(40));
			continue;
		}
		AVPacket *packet = read_pool.pop_packet();
		int ret = -1;
		do
		{
			ret = avcodec_send_packet(codecctx, packet);
			if (ret != 0)
			{
				std::mutex mtx;
				std::unique_lock<std::mutex> lck(mtx);
				std::condition_variable().wait_for(lck, std::chrono::microseconds(10));
			}
		}while(b_running && !b_pausing && ret != 0);

		AVFrame *av_frame = av_frame_alloc();
		if (av_frame == NULL)
		{
			printf("av_frame_alloc failed\n");
			continue;
		}
		ret = avcodec_receive_frame(codecctx, av_frame);
		if (ret == 0)
		{
			int push_success = -1;
			do {
				push_success = render_pool.push_frame(av_frame);
				if (push_success != 0)
				{
					decode_thrd.cond.wait_for(lck, std::chrono::microseconds(40));
				}
				else
				{
					render_thrd.cond.notify_all();
				}
			} while (b_running && !b_pausing && push_success != 0);
		}
		else
		{
			av_frame_free(&av_frame);
		}
		read_pool.free_packet(packet);
	}
	printf("video decode thread exit\n");
}

void SaveAvFrame(AVFrame *avFrame)
{
	static int count = 0;
	char name[256];
	sprintf(name, "testyuv_%d", count);
	count++;
	FILE *fDump = fopen(name, "wb+");

	uint32_t pitchY = avFrame->linesize[0];
	uint32_t pitchU = avFrame->linesize[1];
	uint32_t pitchV = avFrame->linesize[2];

	uint8_t *avY = avFrame->data[0];
	uint8_t *avU = avFrame->data[1];
	uint8_t *avV = avFrame->data[2];

	for (uint32_t i = 0; i < avFrame->height; i++) {
		fwrite(avY, avFrame->width, 1, fDump);
		avY += pitchY;
	}

	for (uint32_t i = 0; i < avFrame->height / 2; i++) {
		fwrite(avU, avFrame->width / 2, 1, fDump);
		avU += pitchU;
	}

	for (uint32_t i = 0; i < avFrame->height / 2; i++) {
		fwrite(avV, avFrame->width / 2, 1, fDump);
		avV += pitchV;
	}

	fclose(fDump);
}

void codec_instance::video_render_proc(void)
{
	std::unique_lock<std::mutex> lck(render_thrd.mtx);
	unsigned int fps = fc->streams[videoindex]->avg_frame_rate.num / fc->streams[videoindex]->avg_frame_rate.den;
	unsigned int delay_time = 1000 / fps;
	int cur_width = 0, cur_height = 0;
	while (b_running)
	{
		if (b_pausing)
		{
			render_thrd.cond.wait_for(lck, std::chrono::microseconds(40));
			continue;
		}
		if (render_pool.peek_frame() != 0)
		{
			decode_thrd.cond.notify_all();
			render_thrd.cond.wait_for(lck, std::chrono::microseconds(40));
			continue;
		}

		AVFrame *frame = render_pool.pop_frame();

		//SaveAvFrame(frame);
		auto now = std::chrono::steady_clock::now();
		auto time_diff = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_render_time).count();

		if (time_diff < delay_time)
		{
			std::mutex mtx;
			std::unique_lock<std::mutex> lck(mtx);
			std::condition_variable().wait_for(lck, std::chrono::microseconds(delay_time - time_diff));
		}
		last_render_time = std::chrono::steady_clock::now();

		if ((cur_width != frame->width) || (cur_height != frame->height))
		{
			if (render_ins)
			{
				render_ins->deinit_video_render();
				delete render_ins;
			}
			render_ins = new render_instance();
			render_ins->init_video_render(_hwnd, frame->width, frame->height);
			cur_width = frame->width;
			cur_height = frame->height;
		}

		render_ins->Render_g(frame->data, frame->width, frame->height);

		decode_thrd.cond.notify_all();
		render_pool.free_frame(frame);

	}
	printf("video render thread exit\n");
}

void codec_instance::set_x_angle(int angle)
{
	if (render_ins)
	{
		render_ins->set_x_angle(angle);
	}
}
void codec_instance::set_y_angle(int angle)
{
	if (render_ins)
	{
		render_ins->set_y_angle(angle);
	}
}

void codec_instance::set_fov_angle(int angle)
{
	if (render_ins)
	{
		render_ins->set_fov_angle(angle);
	}
}

void codec_instance::zoom(float ratio)
{
	if (render_ins)
	{
		render_ins->zoom(ratio);
	}
}

int packet_pool::push_packet(AVPacket *packet)
{
	if ((packet->size + store_buf_size) > MAX_AVPACKET_POOL_SIZE)
	{
		return -1;
	}
	std::lock_guard<std::recursive_mutex> lck(mtx);
	avfrm_list.push_back(packet);
	store_buf_size += packet->size;
	return 0;
}
int packet_pool::peek_packet()
{
	std::lock_guard<std::recursive_mutex> lck(mtx);
	if (!avfrm_list.empty())
	{
		return 0;
	}
	return -1;
}
AVPacket * packet_pool::pop_packet()
{
	std::lock_guard<std::recursive_mutex> lck(mtx);
	AVPacket *ret = avfrm_list.front();
	avfrm_list.pop_front();
	store_buf_size -= ret->size;
	return ret;
}
void packet_pool::free_packet(AVPacket *packet)
{
	if (packet)
	{
		av_packet_unref(packet);
	}
}

void packet_pool::clear_packet()
{
	std::lock_guard<std::recursive_mutex> lck(mtx);
	for (auto iter:avfrm_list)
	{
		if (iter)
		{
			av_packet_unref(iter);
		}
	}
	avfrm_list.clear();
	store_buf_size = 0;
}

int avframe_pool::push_frame(AVFrame *frame)
{
	if ((store_num) >= MAX_AVFRAME_POOL_NUM)
	{
		return -1;
	}
	std::lock_guard<std::recursive_mutex> lck(mtx);
	avfrm_list.push_back(frame);
	store_num ++;
	return 0;
}

int avframe_pool::peek_frame()
{
	std::lock_guard<std::recursive_mutex> lck(mtx);
	if (!avfrm_list.empty())
	{
		return 0;
	}
	return -1;
}
AVFrame * avframe_pool::pop_frame()
{
	std::lock_guard<std::recursive_mutex> lck(mtx);
	AVFrame *ret = avfrm_list.front();
	avfrm_list.pop_front();
	store_num --;
	return ret;
}
void avframe_pool::free_frame(AVFrame *frame)
{
	if (frame)
	{
		av_frame_free(&frame);
	}
}
void avframe_pool::clear_frame()
{
	std::lock_guard<std::recursive_mutex> lck(mtx);
	for (auto iter : avfrm_list)
	{
		if (iter)
		{
			av_frame_free(&iter);
		}
	}
	avfrm_list.clear();
	store_num = 0;
}