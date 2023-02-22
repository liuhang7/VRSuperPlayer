#include "render_instance.h"
#include <stdio.h>
#include <math.h>
#include <cmath>
#include <corecrt_math_defines.h>

//顶点着色器，每个顶点执行一次，可以并行执行
#define GET_STR(x) #x
static const char *vertexShader = GET_STR(
	attribute vec4 aPosition;//输入的顶点坐标，会在程序指定将数据输入到该字段
	attribute vec2 aTextCoord;//输入的纹理坐标，会在程序指定将数据输入到该字段
	varying vec2 vTextCoord;//输出的纹理坐标
	void main() {
		//这里其实是将上下翻转过来（因为安卓图片会自动上下翻转，所以转回来）
		vTextCoord = vec2(aTextCoord.x, 1.0 - aTextCoord.y);
		//直接把传入的坐标值作为传入渲染管线。gl_Position是OpenGL内置的
		gl_Position = aPosition;
	}
);
//图元被光栅化为多少片段，就被调用多少次
static const char *fragYUV420P = GET_STR(
	varying vec2 vTextCoord;
	//输入的yuv三个纹理
	uniform sampler2D yTexture;//采样器
	uniform sampler2D uTexture;//采样器
	uniform sampler2D vTexture;//采样器
	void main() {
		vec3 yuv;
		vec3 rgb;
		//分别取yuv各个分量的采样纹理（r表示？）
		//
		yuv.x = texture2D(yTexture, vTextCoord).r;
		yuv.y = texture2D(uTexture, vTextCoord).r - 0.5;
		yuv.z = texture2D(vTexture, vTextCoord).r - 0.5;
		rgb = mat3(
			1.0, 1.0, 1.0,
			0.0, -0.39465, 2.03211,
			1.13983, -0.5806, 0.0
		) * yuv;
		//gl_FragColor是OpenGL内置的
		gl_FragColor = vec4(rgb, 1.0);
	}
);

//加入三维顶点数据
static float ver[] = {
		1.0f, -1.0f, 0.0f,
		-1.0f, -1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,
		-1.0f, 1.0f, 0.0f
};

//加入纹理坐标数据
static float fragment[] = {
		1.0f, 0.0f,
		0.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 1.0f
};

GLint render_instance::initShader(const char *source, GLint type) 
{
	//创建shader
	GLint sh = glCreateShader(type);
	if (sh == 0) {
		printf("glCreateShader %d failed", type);
		return 0;
	}
	//加载shader
	glShaderSource(sh,
		1,//shader数量
		&source,
		0);//代码长度，传0则读到字符串结尾

//编译shader
	glCompileShader(sh);

	GLint status;
	glGetShaderiv(sh, GL_COMPILE_STATUS, &status);
	if (status == 0) {
		printf("glCompileShader %d failed", type);
		printf("source %s", source);
		return 0;
	}

	printf("glCompileShader %d success", type);
	return sh;
}

int render_instance::init_normal_program(void)
{
	printf("vertex shader:\n");
	printf("%s\n", vertexShader);
	printf("frag shader:\n");
	printf("%s\n", fragYUV420P);
	vsh = initShader(vertexShader, GL_VERTEX_SHADER);
	fsh = initShader(fragYUV420P, GL_FRAGMENT_SHADER);

	//创建渲染程序
	program = glCreateProgram();
	if (program == 0) {
		printf("glCreateProgram failed");
		return -1;
	}

	//向渲染程序中加入着色器
	glAttachShader(program, vsh);
	glAttachShader(program, fsh);

	//链接程序
	glLinkProgram(program);
	GLint status = 0;
	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (status == 0) {
		printf("glLinkProgram failed");
		return -1;
	}
	printf("glLinkProgram success");

	GLuint apos = static_cast<GLuint>(glGetAttribLocation(program, "aPosition"));
	glVertexAttribPointer(apos, 3, GL_FLOAT, GL_FALSE, 0, ver);
	glEnableVertexAttribArray(apos);


	GLuint aTex = static_cast<GLuint>(glGetAttribLocation(program, "aTextCoord"));
	glVertexAttribPointer(aTex, 2, GL_FLOAT, GL_FALSE, 0, fragment);
	glEnableVertexAttribArray(aTex);


	uniform[0] = glGetUniformLocation(program, "yTexture");
	uniform[1] = glGetUniformLocation(program, "uTexture");
	uniform[2] = glGetUniformLocation(program, "vTexture");

	return 0;
}

int render_instance::init_yuv_texture(int width, int height)
{
	//纹理ID

	//创建若干个纹理对象，并且得到纹理ID
	glGenTextures(3, texts);

	//绑定纹理。后面的的设置和加载全部作用于当前绑定的纹理对象
	//GL_TEXTURE0、GL_TEXTURE1、GL_TEXTURE2 的就是纹理单元，GL_TEXTURE_1D、GL_TEXTURE_2D、CUBE_MAP为纹理目标
	//通过 glBindTexture 函数将纹理目标和纹理绑定后，对纹理目标所进行的操作都反映到对纹理上
	glBindTexture(GL_TEXTURE_2D, texts[0]);
	//缩小的过滤器
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	//放大的过滤器
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	//设置纹理的格式和大小
	// 加载纹理到 OpenGL，读入 buffer 定义的位图数据，并把它复制到当前绑定的纹理对象
	// 当前绑定的纹理对象就会被附加上纹理图像。
	//width,height表示每几个像素公用一个yuv元素？比如width / 2表示横向每两个像素使用一个元素？
	glTexImage2D(GL_TEXTURE_2D,
		0,//细节基本 默认0
		GL_RED,//gpu内部格式 亮度，灰度图（这里就是只取一个亮度的颜色通道的意思）
		width,//加载的纹理宽度。最好为2的次幂(这里对y分量数据当做指定尺寸算，但显示尺寸会拉伸到全屏？)
		height,//加载的纹理高度。最好为2的次幂
		0,//纹理边框
		GL_RED,//数据的像素格式 亮度，灰度图
		GL_UNSIGNED_BYTE,//像素点存储的数据类型
		NULL //纹理的数据（先不传）
	);

	//绑定纹理
	glBindTexture(GL_TEXTURE_2D, texts[1]);
	//缩小的过滤器
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	//设置纹理的格式和大小
	glTexImage2D(GL_TEXTURE_2D,
		0,//细节基本 默认0
		GL_RED,//gpu内部格式 亮度，灰度图（这里就是只取一个颜色通道的意思）
		width / 2,//u数据数量为屏幕的4分之1
		height / 2,
		0,//边框
		GL_RED,//数据的像素格式 亮度，灰度图
		GL_UNSIGNED_BYTE,//像素点存储的数据类型
		NULL //纹理的数据（先不传）
	);

	//绑定纹理
	glBindTexture(GL_TEXTURE_2D, texts[2]);
	//缩小的过滤器
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	//设置纹理的格式和大小
	glTexImage2D(GL_TEXTURE_2D,
		0,//细节基本 默认0
		GL_RED,//gpu内部格式 亮度，灰度图（这里就是只取一个颜色通道的意思）
		width / 2,
		height / 2,//v数据数量为屏幕的4分之1
		0,//边框
		GL_RED,//数据的像素格式 亮度，灰度图
		GL_UNSIGNED_BYTE,//像素点存储的数据类型
		NULL //纹理的数据（先不传）
	);
	return 0;
}

static const char *vertexShader_g = GET_STR(
	uniform mat4 uMVPMatrix;
	attribute vec4 aPosition;//输入的顶点坐标，会在程序指定将数据输入到该字段
	attribute vec2 aTextCoord;//输入的纹理坐标，会在程序指定将数据输入到该字段
	varying vec2 vTextCoord;//输出的纹理坐标
	void main() {
		//这里其实是将上下翻转过来（因为安卓图片会自动上下翻转，所以转回来）
		vTextCoord = aTextCoord;
		//直接把传入的坐标值作为传入渲染管线。gl_Position是OpenGL内置的
		gl_Position = uMVPMatrix * aPosition;
	}
);
//图元被光栅化为多少片段，就被调用多少次
static const char *fragYUV420P_g = GET_STR(

	varying vec2 vTextCoord;
	//输入的yuv三个纹理
	uniform sampler2D yTexture;//采样器
	uniform sampler2D uTexture;//采样器
	uniform sampler2D vTexture;//采样器
	void main() {
		vec3 yuv;
		vec3 rgb;
		//分别取yuv各个分量的采样纹理（r表示？）
		//
		yuv.x = texture2D(yTexture, vTextCoord).r;
		yuv.y = texture2D(uTexture, vTextCoord).r - 0.5;
		yuv.z = texture2D(vTexture, vTextCoord).r - 0.5;
		rgb = mat3(
			1.0, 1.0, 1.0,
			0.0, -0.39465, 2.03211,
			1.13983, -0.5806, 0.0
		) * yuv;
		//gl_FragColor是OpenGL内置的
		gl_FragColor = vec4(rgb, 1.0);
	}
);

#define CAP (4)
#define weiduNUM (180 / CAP)
#define jinduNUM (360 / CAP)

int render_instance::init_global_program()
{
	printf("vertex shader:\n");
	printf("%s\n", vertexShader_g);
	printf("frag shader:\n");
	printf("%s\n", fragYUV420P_g);
	vsh_g = initShader(vertexShader_g, GL_VERTEX_SHADER);
	fsh_g = initShader(fragYUV420P_g, GL_FRAGMENT_SHADER);

	//创建渲染程序
	program_g = glCreateProgram();
	if (program_g == 0) {
		printf("glCreateProgram failed");
		return -1;
	}

	//向渲染程序中加入着色器
	glAttachShader(program_g, vsh_g);
	glAttachShader(program_g, fsh_g);

	//链接程序
	glLinkProgram(program_g);
	GLint status = 0;
	glGetProgramiv(program_g, GL_LINK_STATUS, &status);
	if (status == 0) {
		printf("glLinkProgram failed");
		return -1;
	}
	printf("glLinkProgram success");

	point_vertex = (float *)malloc((GLsizei)weiduNUM * jinduNUM * 6 * 3 * sizeof(float));

	float x = 0;
	float y = 0;
	float z = 0;

	int index = 0;
	int index1 = 0;


	//y轴指向上，z轴指向右， x轴指向自己
	float *verticals = (float *)point_vertex;
	float r = 1.0f;//球体半径
	double d = CAP * M_PI / 180;//每次递增的弧度
	for (int i = 0; i < 180; i += CAP) {
		double d1 = i * M_PI / 180;
		for (int j = 0; j < 360; j += CAP) {
			//获得球体上切分的超小片矩形的顶点坐标（两个三角形组成，所以有六点顶点）
			double d2 = j * M_PI / 180;
			// d1,d2+d    d1+d,d2+d

			// d1 ,d2     d1+d,d2
			verticals[index++] = (float)(x + r * sin(d1 + d) * cos(d2 + d));
			verticals[index++] = (float)(y + r * cos(d1 + d));
			verticals[index++] = (float)(z + r * sin(d1 + d) * sin(d2 + d));

			verticals[index++] = (float)(x + r * sin(d1) * cos(d2));
			verticals[index++] = (float)(y + r * cos(d1));
			verticals[index++] = (float)(z + r * sin(d1) * sin(d2));

			verticals[index++] = (float)(x + r * sin(d1) * cos(d2 + d));
			verticals[index++] = (float)(y + r * cos(d1));
			verticals[index++] = (float)(z + r * sin(d1) * sin(d2 + d));

			verticals[index++] = (float)(x + r * sin(d1 + d) * cos(d2 + d));
			verticals[index++] = (float)(y + r * cos(d1 + d));
			verticals[index++] = (float)(z + r * sin(d1 + d) * sin(d2 + d));


			verticals[index++] = (float)(x + r * sin(d1 + d) * cos(d2));
			verticals[index++] = (float)(y + r * cos(d1 + d));
			verticals[index++] = (float)(z + r * sin(d1 + d) * sin(d2));

			verticals[index++] = (float)(x + r * sin(d1) * cos(d2));
			verticals[index++] = (float)(y + r * cos(d1));
			verticals[index++] = (float)(z + r * sin(d1) * sin(d2));

		}
	}
	tex_vertex = (float *)malloc((GLsizei)weiduNUM * jinduNUM * 6 * 2 * sizeof(float));
	x = 0;
	y = 0;
	z = 0;
	index = 0;
	index1 = 0;

	verticals = (float *)tex_vertex;
	for (int i = 0; i < 180; i += CAP) {
		double d1 = i * M_PI / 180;
		for (int j = 0; j < 360; j += CAP) {
			//获得球体上切分的超小片矩形的顶点坐标（两个三角形组成，所以有六点顶点）
			double d2 = j * M_PI / 180;

			verticals[index1++] = (j + CAP) * 1.0f / 360;
			verticals[index1++] = (i + CAP) * 1.0f / 180;

			verticals[index1++] = (j) * 1.0f / 360;
			verticals[index1++] = (i) * 1.0f / 180;

			verticals[index1++] = (j + CAP) * 1.0f / 360;
			verticals[index1++] = (i) * 1.0f / 180;

			verticals[index1++] = (j + CAP) * 1.0f / 360;
			verticals[index1++] = (i + CAP) * 1.0f / 180;

			verticals[index1++] = (j) * 1.0f / 360;
			verticals[index1++] = (i + CAP) * 1.0f / 180;

			verticals[index1++] = (j) * 1.0f / 360;
			verticals[index1++] = (i) * 1.0f / 180;
		}
	}
	GLuint apos = static_cast<GLuint>(glGetAttribLocation(program_g, "aPosition"));
	glVertexAttribPointer(apos, 3, GL_FLOAT, GL_FALSE, 0, point_vertex);
	glEnableVertexAttribArray(apos);


	GLuint aTex = static_cast<GLuint>(glGetAttribLocation(program_g, "aTextCoord"));
	glVertexAttribPointer(aTex, 2, GL_FLOAT, GL_FALSE, 0, tex_vertex);
	glEnableVertexAttribArray(aTex);

	uniform_g[0] = glGetUniformLocation(program_g, "yTexture");
	uniform_g[1] = glGetUniformLocation(program_g, "uTexture");
	uniform_g[2] = glGetUniformLocation(program_g, "vTexture");
	mMatrixHandle= glGetUniformLocation(program_g, "uMVPMatrix");


	
	glEnable(GL_DEPTH_TEST);
	//glDisable(GL_CULL_FACE);    //禁用裁剪
	return 0;
}


int render_instance::init_video_render(HWND hwnd, int width, int height)
{
	PIXELFORMATDESCRIPTOR pfd;
	memset(&pfd, 0x00, sizeof(PIXELFORMATDESCRIPTOR));
	pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cRedBits = 8;
	pfd.cGreenBits = 8;
	pfd.cBlueBits = 8;
	pfd.cColorBits = 32;
	pfd.cDepthBits = 24;
	pfd.cStencilBits = 8;

	m_hDC = GetDC(hwnd);

	int nPixelFormat = ChoosePixelFormat(m_hDC, &pfd);

	SetPixelFormat(m_hDC, nPixelFormat, &pfd);

	m_hRC = wglCreateContext(m_hDC);

	wglMakeCurrent(m_hDC, m_hRC);

	gl3wInit();

	m_hwnd = hwnd;
	//init_normal_program();

	////激活渲染程序
	//glUseProgram(program);
	init_global_program();
	glUseProgram(program_g);

	init_yuv_texture(width, height);

	return 0;
}

void render_instance::Render(unsigned char **pdata, unsigned int width, unsigned int height)
{
	int w, h;
	RECT rect;
	::GetWindowRect(m_hwnd, &rect);
	w = rect.right - rect.left;
	h = rect.bottom - rect.top;
	glViewport(0, 0, w, h);

	glClearColor(1.0, 0.0, 0.0, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	
	//激活第一层纹理，绑定到创建的纹理
		//下面的width,height主要是显示尺寸？
	glActiveTexture(GL_TEXTURE0);
	//绑定y对应的纹理
	glBindTexture(GL_TEXTURE_2D, texts[0]);
	//替换纹理，比重新使用glTexImage2D性能高多
	glTexSubImage2D(GL_TEXTURE_2D, 0,
		0, 0,//相对原来的纹理的offset
		width, height,//加载的纹理宽度、高度。最好为2的次幂
		GL_RED, GL_UNSIGNED_BYTE,
		pdata[0]);
	glUniform1i(uniform[0], 0);

	//激活第二层纹理，绑定到创建的纹理
	glActiveTexture(GL_TEXTURE1);
	//绑定u对应的纹理
	glBindTexture(GL_TEXTURE_2D, texts[1]);
	//替换纹理，比重新使用glTexImage2D性能高
	glTexSubImage2D(GL_TEXTURE_2D, 0,
		0, 0,//相对原来的纹理的offset
		width/2, height/2,//加载的纹理宽度、高度。最好为2的次幂
		GL_RED, GL_UNSIGNED_BYTE,
		pdata[1]);
	glUniform1i(uniform[1], 1);

	//激活第三层纹理，绑定到创建的纹理
	glActiveTexture(GL_TEXTURE2);
	//绑定v对应的纹理
	glBindTexture(GL_TEXTURE_2D, texts[2]);
	//替换纹理，比重新使用glTexImage2D性能高
	glTexSubImage2D(GL_TEXTURE_2D, 0,
		0, 0,//相对原来的纹理的offset
		width / 2, height / 2,//加载的纹理宽度、高度。最好为2的次幂
		GL_RED, GL_UNSIGNED_BYTE,
		pdata[2]);
	glUniform1i(uniform[2], 2);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glFlush();
	SwapBuffers(m_hDC);
}

void render_instance::Render_g(unsigned char **pdata, unsigned int width, unsigned int height)
{
	int w, h;
	RECT rect;
	::GetWindowRect(m_hwnd, &rect);
	w = rect.right - rect.left;
	h = rect.bottom - rect.top;
	//printf("#####render_g#####\n");
	glViewport(0, 0, w, h);

	glClearColor(0.0, 0.0, 0.0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (w < h) {
		float ratio = h * 1.0f / w;
		mProjectMatrix = vmath::frustum(-1, 1, -ratio, ratio, 1.0f, 1000.0f);
	}
	else {
		float ratio = w * 1.0f / h;
		mProjectMatrix = vmath::perspective(fov_angle, ratio, 0.1f, 1000);
	}


	float diffx = (eye_x_angle) * 1.0f;
	float diffy = (eye_y_angle) * 1.0f;

	vmath::vec3 up(0, 1, 0);
	vmath::vec3 front(0, 0, 0);
	vmath::vec3 camera(0, 0, zoom_ratio);

	mCameraMatrix = vmath::lookat<float>(camera,front,up);

	//mCameraMatrix = mCameraMatrix.identity();
	//mCameraMatrix = vmath::translate(vmath::vec3(0, 0, -3)) * mCameraMatrix;
	vmath::mat4 model;
	model = model.identity();
	model = vmath::rotate(diffy, vmath::vec3(1, 0, 0)) * model;
	model = vmath::rotate(diffx, vmath::vec3(0, 1, 0)) * model;
	//调整摄像机焦点位置，使画面滚动

	mMVPMatrix = mProjectMatrix * mCameraMatrix * model;// vmath::matrixCompMult(mCameraMatrix, model);


	//mMVPMatrix = vmath::matrixCompMult(mProjectMatrix, mMVPMatrix);
	glUniformMatrix4fv(mMatrixHandle, 1, GL_FALSE, mMVPMatrix);


	//激活第一层纹理，绑定到创建的纹理
		//下面的width,height主要是显示尺寸？
	glActiveTexture(GL_TEXTURE0);
	//绑定y对应的纹理
	glBindTexture(GL_TEXTURE_2D, texts[0]);
	//替换纹理，比重新使用glTexImage2D性能高多
	glTexSubImage2D(GL_TEXTURE_2D, 0,
		0, 0,//相对原来的纹理的offset
		width, height,//加载的纹理宽度、高度。最好为2的次幂
		GL_RED, GL_UNSIGNED_BYTE,
		pdata[0]);
	glUniform1i(uniform_g[0], 0);

	//激活第二层纹理，绑定到创建的纹理
	glActiveTexture(GL_TEXTURE1);
	//绑定u对应的纹理
	glBindTexture(GL_TEXTURE_2D, texts[1]);
	//替换纹理，比重新使用glTexImage2D性能高
	glTexSubImage2D(GL_TEXTURE_2D, 0,
		0, 0,//相对原来的纹理的offset
		width / 2, height / 2,//加载的纹理宽度、高度。最好为2的次幂
		GL_RED, GL_UNSIGNED_BYTE,
		pdata[1]);
	glUniform1i(uniform_g[1], 1);

	//激活第三层纹理，绑定到创建的纹理
	glActiveTexture(GL_TEXTURE2);
	//绑定v对应的纹理
	glBindTexture(GL_TEXTURE_2D, texts[2]);
	//替换纹理，比重新使用glTexImage2D性能高
	glTexSubImage2D(GL_TEXTURE_2D, 0,
		0, 0,//相对原来的纹理的offset
		width / 2, height / 2,//加载的纹理宽度、高度。最好为2的次幂
		GL_RED, GL_UNSIGNED_BYTE,
		pdata[2]);
	glUniform1i(uniform_g[2], 2);

	glDrawArrays(GL_TRIANGLES, 0, (GLsizei)weiduNUM * jinduNUM * 6);

	glFlush();
	SwapBuffers(m_hDC);
}


int render_instance::deinit_video_render(void)
{
	glDeleteTextures(3, texts);
	if (vsh)
	{
		glDetachShader(program, vsh);
		glDeleteShader(vsh);
		vsh = 0;
	}

	if (fsh)
	{
		glDetachShader(program, fsh);
		glDeleteShader(fsh);
		fsh = 0;
	}
	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(m_hRC);
	ReleaseDC(m_hwnd, m_hDC);
	return 0;
}

void render_instance::zoom(float ratio)
{
	zoom_ratio += ratio;
}