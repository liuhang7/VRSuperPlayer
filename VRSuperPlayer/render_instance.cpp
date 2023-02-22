#include "render_instance.h"
#include <stdio.h>
#include <math.h>
#include <cmath>
#include <corecrt_math_defines.h>

//������ɫ����ÿ������ִ��һ�Σ����Բ���ִ��
#define GET_STR(x) #x
static const char *vertexShader = GET_STR(
	attribute vec4 aPosition;//����Ķ������꣬���ڳ���ָ�����������뵽���ֶ�
	attribute vec2 aTextCoord;//������������꣬���ڳ���ָ�����������뵽���ֶ�
	varying vec2 vTextCoord;//�������������
	void main() {
		//������ʵ�ǽ����·�ת��������Ϊ��׿ͼƬ���Զ����·�ת������ת������
		vTextCoord = vec2(aTextCoord.x, 1.0 - aTextCoord.y);
		//ֱ�ӰѴ��������ֵ��Ϊ������Ⱦ���ߡ�gl_Position��OpenGL���õ�
		gl_Position = aPosition;
	}
);
//ͼԪ����դ��Ϊ����Ƭ�Σ��ͱ����ö��ٴ�
static const char *fragYUV420P = GET_STR(
	varying vec2 vTextCoord;
	//�����yuv��������
	uniform sampler2D yTexture;//������
	uniform sampler2D uTexture;//������
	uniform sampler2D vTexture;//������
	void main() {
		vec3 yuv;
		vec3 rgb;
		//�ֱ�ȡyuv���������Ĳ�������r��ʾ����
		//
		yuv.x = texture2D(yTexture, vTextCoord).r;
		yuv.y = texture2D(uTexture, vTextCoord).r - 0.5;
		yuv.z = texture2D(vTexture, vTextCoord).r - 0.5;
		rgb = mat3(
			1.0, 1.0, 1.0,
			0.0, -0.39465, 2.03211,
			1.13983, -0.5806, 0.0
		) * yuv;
		//gl_FragColor��OpenGL���õ�
		gl_FragColor = vec4(rgb, 1.0);
	}
);

//������ά��������
static float ver[] = {
		1.0f, -1.0f, 0.0f,
		-1.0f, -1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,
		-1.0f, 1.0f, 0.0f
};

//����������������
static float fragment[] = {
		1.0f, 0.0f,
		0.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 1.0f
};

GLint render_instance::initShader(const char *source, GLint type) 
{
	//����shader
	GLint sh = glCreateShader(type);
	if (sh == 0) {
		printf("glCreateShader %d failed", type);
		return 0;
	}
	//����shader
	glShaderSource(sh,
		1,//shader����
		&source,
		0);//���볤�ȣ���0������ַ�����β

//����shader
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

	//������Ⱦ����
	program = glCreateProgram();
	if (program == 0) {
		printf("glCreateProgram failed");
		return -1;
	}

	//����Ⱦ�����м�����ɫ��
	glAttachShader(program, vsh);
	glAttachShader(program, fsh);

	//���ӳ���
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
	//����ID

	//�������ɸ�������󣬲��ҵõ�����ID
	glGenTextures(3, texts);

	//����������ĵ����úͼ���ȫ�������ڵ�ǰ�󶨵��������
	//GL_TEXTURE0��GL_TEXTURE1��GL_TEXTURE2 �ľ�������Ԫ��GL_TEXTURE_1D��GL_TEXTURE_2D��CUBE_MAPΪ����Ŀ��
	//ͨ�� glBindTexture ����������Ŀ�������󶨺󣬶�����Ŀ�������еĲ�������ӳ����������
	glBindTexture(GL_TEXTURE_2D, texts[0]);
	//��С�Ĺ�����
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	//�Ŵ�Ĺ�����
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	//��������ĸ�ʽ�ʹ�С
	// �������� OpenGL������ buffer �����λͼ���ݣ����������Ƶ���ǰ�󶨵��������
	// ��ǰ�󶨵��������ͻᱻ����������ͼ��
	//width,height��ʾÿ�������ع���һ��yuvԪ�أ�����width / 2��ʾ����ÿ��������ʹ��һ��Ԫ�أ�
	glTexImage2D(GL_TEXTURE_2D,
		0,//ϸ�ڻ��� Ĭ��0
		GL_RED,//gpu�ڲ���ʽ ���ȣ��Ҷ�ͼ���������ֻȡһ�����ȵ���ɫͨ������˼��
		width,//���ص������ȡ����Ϊ2�Ĵ���(�����y�������ݵ���ָ���ߴ��㣬����ʾ�ߴ�����쵽ȫ����)
		height,//���ص�����߶ȡ����Ϊ2�Ĵ���
		0,//����߿�
		GL_RED,//���ݵ����ظ�ʽ ���ȣ��Ҷ�ͼ
		GL_UNSIGNED_BYTE,//���ص�洢����������
		NULL //��������ݣ��Ȳ�����
	);

	//������
	glBindTexture(GL_TEXTURE_2D, texts[1]);
	//��С�Ĺ�����
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	//��������ĸ�ʽ�ʹ�С
	glTexImage2D(GL_TEXTURE_2D,
		0,//ϸ�ڻ��� Ĭ��0
		GL_RED,//gpu�ڲ���ʽ ���ȣ��Ҷ�ͼ���������ֻȡһ����ɫͨ������˼��
		width / 2,//u��������Ϊ��Ļ��4��֮1
		height / 2,
		0,//�߿�
		GL_RED,//���ݵ����ظ�ʽ ���ȣ��Ҷ�ͼ
		GL_UNSIGNED_BYTE,//���ص�洢����������
		NULL //��������ݣ��Ȳ�����
	);

	//������
	glBindTexture(GL_TEXTURE_2D, texts[2]);
	//��С�Ĺ�����
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	//��������ĸ�ʽ�ʹ�С
	glTexImage2D(GL_TEXTURE_2D,
		0,//ϸ�ڻ��� Ĭ��0
		GL_RED,//gpu�ڲ���ʽ ���ȣ��Ҷ�ͼ���������ֻȡһ����ɫͨ������˼��
		width / 2,
		height / 2,//v��������Ϊ��Ļ��4��֮1
		0,//�߿�
		GL_RED,//���ݵ����ظ�ʽ ���ȣ��Ҷ�ͼ
		GL_UNSIGNED_BYTE,//���ص�洢����������
		NULL //��������ݣ��Ȳ�����
	);
	return 0;
}

static const char *vertexShader_g = GET_STR(
	uniform mat4 uMVPMatrix;
	attribute vec4 aPosition;//����Ķ������꣬���ڳ���ָ�����������뵽���ֶ�
	attribute vec2 aTextCoord;//������������꣬���ڳ���ָ�����������뵽���ֶ�
	varying vec2 vTextCoord;//�������������
	void main() {
		//������ʵ�ǽ����·�ת��������Ϊ��׿ͼƬ���Զ����·�ת������ת������
		vTextCoord = aTextCoord;
		//ֱ�ӰѴ��������ֵ��Ϊ������Ⱦ���ߡ�gl_Position��OpenGL���õ�
		gl_Position = uMVPMatrix * aPosition;
	}
);
//ͼԪ����դ��Ϊ����Ƭ�Σ��ͱ����ö��ٴ�
static const char *fragYUV420P_g = GET_STR(

	varying vec2 vTextCoord;
	//�����yuv��������
	uniform sampler2D yTexture;//������
	uniform sampler2D uTexture;//������
	uniform sampler2D vTexture;//������
	void main() {
		vec3 yuv;
		vec3 rgb;
		//�ֱ�ȡyuv���������Ĳ�������r��ʾ����
		//
		yuv.x = texture2D(yTexture, vTextCoord).r;
		yuv.y = texture2D(uTexture, vTextCoord).r - 0.5;
		yuv.z = texture2D(vTexture, vTextCoord).r - 0.5;
		rgb = mat3(
			1.0, 1.0, 1.0,
			0.0, -0.39465, 2.03211,
			1.13983, -0.5806, 0.0
		) * yuv;
		//gl_FragColor��OpenGL���õ�
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

	//������Ⱦ����
	program_g = glCreateProgram();
	if (program_g == 0) {
		printf("glCreateProgram failed");
		return -1;
	}

	//����Ⱦ�����м�����ɫ��
	glAttachShader(program_g, vsh_g);
	glAttachShader(program_g, fsh_g);

	//���ӳ���
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


	//y��ָ���ϣ�z��ָ���ң� x��ָ���Լ�
	float *verticals = (float *)point_vertex;
	float r = 1.0f;//����뾶
	double d = CAP * M_PI / 180;//ÿ�ε����Ļ���
	for (int i = 0; i < 180; i += CAP) {
		double d1 = i * M_PI / 180;
		for (int j = 0; j < 360; j += CAP) {
			//����������зֵĳ�СƬ���εĶ������꣨������������ɣ����������㶥�㣩
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
			//����������зֵĳ�СƬ���εĶ������꣨������������ɣ����������㶥�㣩
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
	//glDisable(GL_CULL_FACE);    //���òü�
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

	////������Ⱦ����
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
	
	//�����һ�������󶨵�����������
		//�����width,height��Ҫ����ʾ�ߴ磿
	glActiveTexture(GL_TEXTURE0);
	//��y��Ӧ������
	glBindTexture(GL_TEXTURE_2D, texts[0]);
	//�滻����������ʹ��glTexImage2D���ܸ߶�
	glTexSubImage2D(GL_TEXTURE_2D, 0,
		0, 0,//���ԭ���������offset
		width, height,//���ص������ȡ��߶ȡ����Ϊ2�Ĵ���
		GL_RED, GL_UNSIGNED_BYTE,
		pdata[0]);
	glUniform1i(uniform[0], 0);

	//����ڶ��������󶨵�����������
	glActiveTexture(GL_TEXTURE1);
	//��u��Ӧ������
	glBindTexture(GL_TEXTURE_2D, texts[1]);
	//�滻����������ʹ��glTexImage2D���ܸ�
	glTexSubImage2D(GL_TEXTURE_2D, 0,
		0, 0,//���ԭ���������offset
		width/2, height/2,//���ص������ȡ��߶ȡ����Ϊ2�Ĵ���
		GL_RED, GL_UNSIGNED_BYTE,
		pdata[1]);
	glUniform1i(uniform[1], 1);

	//��������������󶨵�����������
	glActiveTexture(GL_TEXTURE2);
	//��v��Ӧ������
	glBindTexture(GL_TEXTURE_2D, texts[2]);
	//�滻����������ʹ��glTexImage2D���ܸ�
	glTexSubImage2D(GL_TEXTURE_2D, 0,
		0, 0,//���ԭ���������offset
		width / 2, height / 2,//���ص������ȡ��߶ȡ����Ϊ2�Ĵ���
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
	//�������������λ�ã�ʹ�������

	mMVPMatrix = mProjectMatrix * mCameraMatrix * model;// vmath::matrixCompMult(mCameraMatrix, model);


	//mMVPMatrix = vmath::matrixCompMult(mProjectMatrix, mMVPMatrix);
	glUniformMatrix4fv(mMatrixHandle, 1, GL_FALSE, mMVPMatrix);


	//�����һ�������󶨵�����������
		//�����width,height��Ҫ����ʾ�ߴ磿
	glActiveTexture(GL_TEXTURE0);
	//��y��Ӧ������
	glBindTexture(GL_TEXTURE_2D, texts[0]);
	//�滻����������ʹ��glTexImage2D���ܸ߶�
	glTexSubImage2D(GL_TEXTURE_2D, 0,
		0, 0,//���ԭ���������offset
		width, height,//���ص������ȡ��߶ȡ����Ϊ2�Ĵ���
		GL_RED, GL_UNSIGNED_BYTE,
		pdata[0]);
	glUniform1i(uniform_g[0], 0);

	//����ڶ��������󶨵�����������
	glActiveTexture(GL_TEXTURE1);
	//��u��Ӧ������
	glBindTexture(GL_TEXTURE_2D, texts[1]);
	//�滻����������ʹ��glTexImage2D���ܸ�
	glTexSubImage2D(GL_TEXTURE_2D, 0,
		0, 0,//���ԭ���������offset
		width / 2, height / 2,//���ص������ȡ��߶ȡ����Ϊ2�Ĵ���
		GL_RED, GL_UNSIGNED_BYTE,
		pdata[1]);
	glUniform1i(uniform_g[1], 1);

	//��������������󶨵�����������
	glActiveTexture(GL_TEXTURE2);
	//��v��Ӧ������
	glBindTexture(GL_TEXTURE_2D, texts[2]);
	//�滻����������ʹ��glTexImage2D���ܸ�
	glTexSubImage2D(GL_TEXTURE_2D, 0,
		0, 0,//���ԭ���������offset
		width / 2, height / 2,//���ص������ȡ��߶ȡ����Ϊ2�Ĵ���
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