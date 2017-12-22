#ifndef MYCLIENT_H
#define MYCLIENT_H

#include <QWidget>
#include <QTcpSocket>

class QTextEdit;
class QLineEdit;

namespace Ui {
class MyClient;
}

class MyClient : public QWidget
{
    Q_OBJECT

public:
    //explicit MyClient(QWidget *parent = 0);
    explicit MyClient(const QString& strHost, int nPort, QWidget* pwgt = 0) ;
    ~MyClient();

private:
    Ui::MyClient *ui;



private:
    QTcpSocket* m_pTcpSocket;
    QTextEdit*  m_ptxtInfo;
    QLineEdit*  m_ptxtInput;
    quint16     m_nNextBlockSize;

private slots:
    void slotReadyRead   (                            );
    void slotError       (QAbstractSocket::SocketError);
    void slotSendToServer(                            );
    void slotConnected   (                            );
    void endTask();

private:
    QString localIP();

};

#endif // MYCLIENT_H
