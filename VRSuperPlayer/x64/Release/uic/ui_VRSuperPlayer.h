/********************************************************************************
** Form generated from reading UI file 'VRSuperPlayer.ui'
**
** Created by: Qt User Interface Compiler version 5.9.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_VRSUPERPLAYER_H
#define UI_VRSUPERPLAYER_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSlider>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_VRSuperPlayerClass
{
public:
    QWidget *centralWidget;
    QWidget *VideoWidget;
    QWidget *layoutWidget;
    QFormLayout *formLayout;
    QGridLayout *gridLayout;
    QPushButton *OpenButton;
    QPushButton *PlayButton;
    QLabel *FilePath;
    QWidget *widget;
    QGridLayout *gridLayout_2;
    QLabel *label;
    QSlider *x_slider;
    QLabel *label_2;
    QSlider *y_slider;
    QMenuBar *menuBar;
    QToolBar *mainToolBar;
    QStatusBar *statusBar;

    void setupUi(QMainWindow *VRSuperPlayerClass)
    {
        if (VRSuperPlayerClass->objectName().isEmpty())
            VRSuperPlayerClass->setObjectName(QStringLiteral("VRSuperPlayerClass"));
        VRSuperPlayerClass->resize(1127, 781);
        centralWidget = new QWidget(VRSuperPlayerClass);
        centralWidget->setObjectName(QStringLiteral("centralWidget"));
        VideoWidget = new QWidget(centralWidget);
        VideoWidget->setObjectName(QStringLiteral("VideoWidget"));
        VideoWidget->setGeometry(QRect(0, 0, 1121, 641));
        layoutWidget = new QWidget(centralWidget);
        layoutWidget->setObjectName(QStringLiteral("layoutWidget"));
        layoutWidget->setGeometry(QRect(1, 652, 166, 51));
        formLayout = new QFormLayout(layoutWidget);
        formLayout->setSpacing(6);
        formLayout->setContentsMargins(11, 11, 11, 11);
        formLayout->setObjectName(QStringLiteral("formLayout"));
        formLayout->setContentsMargins(0, 0, 0, 0);
        gridLayout = new QGridLayout();
        gridLayout->setSpacing(6);
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        OpenButton = new QPushButton(layoutWidget);
        OpenButton->setObjectName(QStringLiteral("OpenButton"));

        gridLayout->addWidget(OpenButton, 0, 0, 1, 1);

        PlayButton = new QPushButton(layoutWidget);
        PlayButton->setObjectName(QStringLiteral("PlayButton"));

        gridLayout->addWidget(PlayButton, 0, 1, 1, 1);


        formLayout->setLayout(0, QFormLayout::LabelRole, gridLayout);

        FilePath = new QLabel(layoutWidget);
        FilePath->setObjectName(QStringLiteral("FilePath"));

        formLayout->setWidget(1, QFormLayout::LabelRole, FilePath);

        widget = new QWidget(centralWidget);
        widget->setObjectName(QStringLiteral("widget"));
        widget->setGeometry(QRect(180, 650, 281, 52));
        gridLayout_2 = new QGridLayout(widget);
        gridLayout_2->setSpacing(6);
        gridLayout_2->setContentsMargins(11, 11, 11, 11);
        gridLayout_2->setObjectName(QStringLiteral("gridLayout_2"));
        gridLayout_2->setContentsMargins(0, 0, 0, 0);
        label = new QLabel(widget);
        label->setObjectName(QStringLiteral("label"));

        gridLayout_2->addWidget(label, 0, 0, 1, 1);

        x_slider = new QSlider(widget);
        x_slider->setObjectName(QStringLiteral("x_slider"));
        x_slider->setOrientation(Qt::Horizontal);

        gridLayout_2->addWidget(x_slider, 0, 1, 1, 1);

        label_2 = new QLabel(widget);
        label_2->setObjectName(QStringLiteral("label_2"));

        gridLayout_2->addWidget(label_2, 1, 0, 1, 1);

        y_slider = new QSlider(widget);
        y_slider->setObjectName(QStringLiteral("y_slider"));
        y_slider->setOrientation(Qt::Horizontal);

        gridLayout_2->addWidget(y_slider, 1, 1, 1, 1);

        VRSuperPlayerClass->setCentralWidget(centralWidget);
        menuBar = new QMenuBar(VRSuperPlayerClass);
        menuBar->setObjectName(QStringLiteral("menuBar"));
        menuBar->setGeometry(QRect(0, 0, 1127, 23));
        VRSuperPlayerClass->setMenuBar(menuBar);
        mainToolBar = new QToolBar(VRSuperPlayerClass);
        mainToolBar->setObjectName(QStringLiteral("mainToolBar"));
        VRSuperPlayerClass->addToolBar(Qt::TopToolBarArea, mainToolBar);
        statusBar = new QStatusBar(VRSuperPlayerClass);
        statusBar->setObjectName(QStringLiteral("statusBar"));
        VRSuperPlayerClass->setStatusBar(statusBar);

        retranslateUi(VRSuperPlayerClass);

        QMetaObject::connectSlotsByName(VRSuperPlayerClass);
    } // setupUi

    void retranslateUi(QMainWindow *VRSuperPlayerClass)
    {
        VRSuperPlayerClass->setWindowTitle(QApplication::translate("VRSuperPlayerClass", "VRSuperPlayer", Q_NULLPTR));
        OpenButton->setText(QApplication::translate("VRSuperPlayerClass", "\346\211\223\345\274\200\346\226\207\344\273\266", Q_NULLPTR));
        PlayButton->setText(QApplication::translate("VRSuperPlayerClass", "\346\222\255\346\224\276", Q_NULLPTR));
        FilePath->setText(QApplication::translate("VRSuperPlayerClass", "\346\226\207\344\273\266\350\267\257\345\276\204\357\274\232", Q_NULLPTR));
        label->setText(QApplication::translate("VRSuperPlayerClass", "x\350\275\264\345\244\271\350\247\2220-360", Q_NULLPTR));
        label_2->setText(QApplication::translate("VRSuperPlayerClass", "y\350\275\264\345\244\271\350\247\2220-360", Q_NULLPTR));
    } // retranslateUi

};

namespace Ui {
    class VRSuperPlayerClass: public Ui_VRSuperPlayerClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_VRSUPERPLAYER_H
