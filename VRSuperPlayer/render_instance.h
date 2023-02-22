#pragma once
#include <Windows.h>
#include "GL/gl3w.h"
#include "vmath.h"
class render_instance
{
public:
	render_instance() { ; };
	~render_instance() { ; };
private:
	static GLint initShader(const char *source, GLint type);
public:
	int init_video_render(HWND hwnd, int width, int height);
	void Render(unsigned char **pdata, unsigned int width, unsigned int height);
	void Render_g(unsigned char **pdata, unsigned int width, unsigned int height);
	int deinit_video_render(void);

	int init_yuv_texture(int width, int height);
	int init_normal_program(void);
	int init_global_program(void);

	void set_x_angle(int angle) { eye_x_angle = angle; };
	void set_y_angle(int angle) { eye_y_angle = angle; };
	void set_fov_angle(int angle) { fov_angle = angle; };
	void zoom(float ratio);
private:
	HGLRC    m_hRC;    //OpenGL��Ⱦ�����Ļ������
	HDC      m_hDC;    //Windows�豸�������
	HWND	 m_hwnd;
	GLuint texts[3] = { 0 };
	GLint vsh = 0;
	GLint fsh = 0;
	GLint program = 0;
	GLint uniform[3];

	//ȫ����Ƶ���
	GLint vsh_g = 0;
	GLint fsh_g = 0;
	GLint program_g = 0;
	GLint uniform_g[3];
	float *point_vertex = nullptr;
	float *tex_vertex = nullptr;
	GLuint vao;
	vmath::mat4 mProjectMatrix;
	vmath::mat4 mCameraMatrix;
	vmath::mat4 mMVPMatrix;
	GLint mMatrixHandle;
	float mAngleX = 0;// ��������ڵ�x����
	float mAngleY = 0;// ��������ڵ�y����
	float mAngleZ = 1;// ��������ڵ�z����
	int eye_x_angle = 0;
	int eye_y_angle = 0;
	int fov_angle = 45;
	float zoom_ratio = 2.61421356f;
};

