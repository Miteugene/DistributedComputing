#ifndef MYSERVER_H
#define MYSERVER_H

#include <QWidget>
#include <QMainWindow>

class QTcpServer;
class QTextEdit;
class QTcpSocket;
class QLineEdit;

namespace Ui {
    class MyServer;
}

class MyServer : public QMainWindow
{
    Q_OBJECT

public:
    explicit MyServer(int nPort, QWidget *parent = 0);
    ~MyServer();

private:
    Ui::MyServer *ui;


private:
    QTcpServer* m_ptcpServer;
    QTextEdit*  m_ptxt;
    quint16     m_nNextBlockSize;
    QLineEdit*  m_ptxtInput;

private:
    void sendToClient(QTcpSocket* pSocket, const QString& str);
    void sendTask(QTcpSocket* pClientSocket);
    void addItem(int i, QString str);

public slots:
    virtual void slotNewConnection();
            void slotReadClient();
            void slotSendToClient();
            void readFile();
            void startJob();

};

#endif // MYSERVER_H
