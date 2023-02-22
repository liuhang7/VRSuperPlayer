#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_VRSuperPlayer.h"
#include "codec_instance.h"
#include <qevent.h>
#include <qmouseeventtransition.h>
class VRSuperPlayer : public QMainWindow
{
    Q_OBJECT

public:
    VRSuperPlayer(QWidget *parent = Q_NULLPTR);
	void open_button_process(void);
	void play_button_process(void);
	void x_slider_valuechanged(int value);
	void y_slider_valuechanged(int value);
	void fov_slider_valuechanged(int value);
protected:
	void mouseReleaseEvent(QMouseEvent *e);
	void wheelEvent(QWheelEvent *e); //»¬ÂÖ¹ö¶¯
	void mouseDoubleClickEvent(QMouseEvent *e); //Ë«»÷
	void mousePressEvent(QMouseEvent *e); //µ¥»÷
private:
    Ui::VRSuperPlayerClass ui;
	codec_instance video_decoder;
	int x_angle;
	int y_angle;
};
