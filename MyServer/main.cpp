#include "myserver.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    //MyServer w;
    //w.show();

    MyServer     server(2323);
    server.show();

    return a.exec();
}
