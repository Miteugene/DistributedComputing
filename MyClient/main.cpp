#include "myclient.h"
#include <QApplication>

#include <QtGui>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    //MyClient w;
    //w.show();


    QFile file("ip.txt");
    QString str;

    str = "localhost";

    if(file.open(QIODevice::ReadOnly))
    {
        QByteArray block = file.readAll();
        str = block;
        file.close();
    }

    MyClient     client(str, 2323);
    client.show();

    return a.exec();
}
