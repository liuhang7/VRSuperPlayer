#pragma once
#include <string>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <atomic>
#include <list>
#include "render_instance.h"
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
#include "libavdevice/avdevice.h"
#include "libavutil/imgutils.h"
#include "libavutil/rational.h"
#include "libavutil/avutil.h"
#include "libavutil/time.h"
#include "libavutil/opt.h"
};

#define MAX_AVPACKET_POOL_SIZE (10485760)
class packet_pool
{
public:
	std::list<AVPacket *> avfrm_list;
	std::recursive_mutex mtx;
	int store_buf_size = 0;
	int push_packet(AVPacket *packet);
	int peek_packet();
	AVPacket * pop_packet();
	void free_packet(AVPacket *packet);
	void clear_packet();
};

#define MAX_AVFRAME_POOL_NUM (3)
class avframe_pool
{
public:
	std::list<AVFrame *> avfrm_list;
	std::recursive_mutex mtx;
	int store_num = 0;
	int push_frame(AVFrame *frame);
	int peek_frame();
	AVFrame * pop_frame();
	void free_frame(AVFrame *frame);
	void clear_frame();
};


class codec_instance
{
public:
	codec_instance() { ; }
	~codec_instance() { ; }
	void set_play_filepath(std::string filepath) { this->filepath = filepath; };
	void enable_hwaccel(bool enable) { _b_enable_hwaccel = enable; };
	int open_file();
	int close_file();
	void set_play_hwnd(HWND hwnd) { _hwnd = hwnd; };
	void set_x_angle(int angle);
	void set_y_angle(int angle);
	void set_fov_angle(int angle);
	void zoom(float ratio);
private:

	typedef struct thread_manage_e
	{
		std::thread m_thread;
		std::condition_variable cond;
		std::mutex mtx;
	}thread_manage;
	std::string filepath;
	bool _b_enable_hwaccel = false;
	AVFormatContext *fc = NULL;
	AVCodec *codec = NULL;
	AVCodecContext *codecctx = NULL;
	int videoindex = -1;

	thread_manage file_thrd;
	thread_manage decode_thrd;
	thread_manage render_thrd;

	void file_read_proc(void);
	void video_decode_proc(void);
	void video_render_proc(void);

	std::atomic_bool b_running = false;
	std::atomic_bool b_pausing = false;

	packet_pool read_pool;
	avframe_pool render_pool;

	std::chrono::time_point<std::chrono::steady_clock> last_render_time;
	AVFrame *last_render_frame = nullptr;

	render_instance *render_ins = nullptr;
	HWND _hwnd = NULL;
};

