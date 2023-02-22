#include "VRSuperPlayer.h"
#include <qfiledialog.h>
#include <qdebug.h>
#include <qslider.h>
#pragma execution_character_set("utf-8")
VRSuperPlayer::VRSuperPlayer(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);
	ui.VideoWidget->setAttribute(Qt::WA_StyledBackground, true);
	ui.VideoWidget->setStyleSheet("background-color: rgb(0,0, 0)");
	
	::ShowWindow((HWND)ui.VideoWidget->winId(),SW_SHOW);
	connect(ui.OpenButton, &QPushButton::pressed, this, &VRSuperPlayer::open_button_process);
	connect(ui.PlayButton, &QPushButton::pressed, this, &VRSuperPlayer::play_button_process);
	ui.x_slider->setMinimum(0);
	ui.x_slider->setMaximum(360);
	ui.x_slider->setSingleStep(1);
	ui.x_slider->setValue(0);
	x_angle = 0;
	ui.y_slider->setMinimum(0);
	ui.y_slider->setMaximum(360);
	ui.y_slider->setSingleStep(1);
	ui.y_slider->setValue(0);
	y_angle = 0;
	ui.fov_slider->setMinimum(0);
	ui.fov_slider->setMaximum(90);
	ui.fov_slider->setSingleStep(1);
	ui.fov_slider->setValue(45);
	connect(ui.x_slider, &QSlider::valueChanged,this, &VRSuperPlayer::x_slider_valuechanged);
	connect(ui.y_slider, &QSlider::valueChanged, this, &VRSuperPlayer::y_slider_valuechanged);
	connect(ui.fov_slider, &QSlider::valueChanged, this, &VRSuperPlayer::fov_slider_valuechanged);
}

void VRSuperPlayer::open_button_process(void)
{
	QString FilePath = QFileDialog::getOpenFileName(this, "open", "D://Project",
		"*.*");
	video_decoder.set_play_filepath(FilePath.toStdString());
	ui.FilePath->setText(FilePath);
}

void VRSuperPlayer::play_button_process(void)
{
	video_decoder.set_play_filepath("D://Project//IPVR-055-C.mp4");
	ui.FilePath->setText("D://Project//IPVR-055-C.mp4");

	video_decoder.enable_hwaccel(false);
	printf("#####hwnd = %x\n", ui.VideoWidget->winId());
	video_decoder.set_play_hwnd((HWND)ui.VideoWidget->winId());
	video_decoder.open_file();
}

void VRSuperPlayer::x_slider_valuechanged(int value)
{
	video_decoder.set_x_angle(value);
}
void VRSuperPlayer::y_slider_valuechanged(int value)
{
	video_decoder.set_y_angle(value);
}

void VRSuperPlayer::fov_slider_valuechanged(int value)
{
	video_decoder.set_fov_angle(value);
}

void VRSuperPlayer::mouseReleaseEvent(QMouseEvent *e)
{
	int x = e->x();
	int y = e->y();
	int w = ui.VideoWidget->width();
	int h = ui.VideoWidget->height();
	int angle_ratio = 5;
	if ((float)x / w < 0.2)
	{
		x_angle -= angle_ratio;
	}
	else if((float)x / w > 0.8)
	{
		x_angle += angle_ratio;
	}

	if ((float)y / h < 0.2)
	{
		y_angle -= angle_ratio;
	}
	else if ((float)y / h > 0.8)
	{
		y_angle += angle_ratio;
	}
	if (x_angle < 0) x_angle = 0;
	if (y_angle < 0) y_angle = 0;
	if (x_angle > 360) x_angle = 360;
	if (y_angle > 360) y_angle = 360;

	ui.x_slider->setValue(x_angle);
	ui.y_slider->setValue(y_angle);
	x_slider_valuechanged(x_angle);
	y_slider_valuechanged(y_angle);
}

void VRSuperPlayer::mousePressEvent(QMouseEvent *e) //µ¥»÷
{
	
}

void VRSuperPlayer::wheelEvent(QWheelEvent *e) //»¬ÂÖ¹ö¶¯
{
	printf("zoom\n");
	float delta = 0.2;
	if (e->delta() > 0) {
		video_decoder.zoom(-delta);
	}
	else {
		video_decoder.zoom(delta);
	}
}
void VRSuperPlayer::mouseDoubleClickEvent(QMouseEvent *e) //Ë«»÷
{

}