#include "VRSuperPlayer.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    VRSuperPlayer w;
    w.show();
    return a.exec();
}
