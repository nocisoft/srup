#include <QtGui/QApplication>
#include "srupmainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
	a.setQuitOnLastWindowClosed(true);
    SRUPMainWindow w;
    w.show();
    return a.exec();
}
