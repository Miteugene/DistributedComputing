#include "myclient.h"
#include "ui_myclient.h"

#include <QtNetwork>
#include <QtGui>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>

#include <QNetworkInterface>

QProcess* process;
QString tstr;
QString numberTask;

QDateTime start_time;
QDateTime finish_time;
QTime t;

MyClient::MyClient(const QString&   strHost,
                   int              nPort,
                   QWidget          *parent) :
    QWidget(parent),
    ui(new Ui::MyClient),
    m_nNextBlockSize(0)
{
    ui->setupUi(this);

    m_pTcpSocket = new QTcpSocket(this);

    m_pTcpSocket->connectToHost(strHost, nPort);
    connect(m_pTcpSocket, SIGNAL(connected()), SLOT(slotConnected()));
    connect(m_pTcpSocket, SIGNAL(readyRead()), SLOT(slotReadyRead()));
    connect(m_pTcpSocket, SIGNAL(error(QAbstractSocket::SocketError)),
            this,         SLOT(slotError(QAbstractSocket::SocketError))
           );

    m_ptxtInfo  = new QTextEdit;
    m_ptxtInput = new QLineEdit;

    connect(m_ptxtInput, SIGNAL(returnPressed()),
            this,        SLOT(slotSendToServer())
           );
    m_ptxtInfo->setReadOnly(true);

    QPushButton* pcmd = new QPushButton("&Send");
    connect(pcmd, SIGNAL(clicked()), SLOT(slotSendToServer()));

    //Layout setup
    QVBoxLayout* pvbxLayout = new QVBoxLayout;
    pvbxLayout->addWidget(new QLabel("<H1>Client</H1>"));
    pvbxLayout->addWidget(m_ptxtInfo);
    pvbxLayout->addWidget(m_ptxtInput);
    pvbxLayout->addWidget(pcmd);
    setLayout(pvbxLayout);


    process = new QProcess(this);

    connect (process, SIGNAL(readyReadStandardOutput()), this, SLOT(endTask()) );
    connect (process, SIGNAL(readyReadStandardError()), this, SLOT(endTask()) );
}

MyClient::~MyClient()
{
    delete ui;
}


// ----------------------------------------------------------------------
void MyClient::slotReadyRead()
{
    QDataStream in(m_pTcpSocket);
    in.setVersion(QDataStream::Qt_5_10);
    for (;;) {
        if (!m_nNextBlockSize) {
            if (m_pTcpSocket->bytesAvailable() < sizeof(quint16)) {
                break;
            }
            in >> m_nNextBlockSize;
        }

        if (m_pTcpSocket->bytesAvailable() < m_nNextBlockSize) {
            break;
        }

        QTime   time;
        QString str;
        in >> time >> str;

        m_nNextBlockSize = 0;

        str.replace("From list ", "");
        str.replace("\"", "");

        tstr = str;

        if (str.contains("--", Qt::CaseSensitive)) {
            m_ptxtInfo->append("\n\nServer: " + str);

            QStringList numberTaskList = str.split("--");
            numberTask = numberTaskList.at(0);
            numberTaskList.removeAt(0);
            str = numberTaskList.at(0);



            QStringList strList = str.split(' ');
            QString pr = strList.at(0);

            strList.removeAt(0);

            process->waitForFinished();


            start_time = QDateTime::currentDateTime();
            t.start();

            process->start(pr, strList);

        }
    }



}

void MyClient::endTask()
{
    QString res = process->readAll();
    res.chop(2);

    finish_time = QDateTime::currentDateTime();
    int el = t.elapsed();

    res = tstr + "\n\t" + res;
    m_ptxtInfo->append("\n\nClient: " + res);
    m_ptxtInput->setText("Client: " + res);
    slotSendToServer();
    m_ptxtInput->setText("Client: endTask: " +
                         numberTask + "|" +
                         start_time.toString("hh:mm:ss") + "|" +
                         finish_time.toString("hh:mm:ss") + "|" +
                         QString::number(el) + "|" +
                         localIP()
                         );
    slotSendToServer();
}

// ----------------------------------------------------------------------
void MyClient::slotError(QAbstractSocket::SocketError err)
{
    QString strError =
        "Error: " + (err == QAbstractSocket::HostNotFoundError ?
                     "The host was not found." :
                     err == QAbstractSocket::RemoteHostClosedError ?
                     "The remote host is closed." :
                     err == QAbstractSocket::ConnectionRefusedError ?
                     "The connection was refused." :
                     QString(m_pTcpSocket->errorString())
                    );
    m_ptxtInfo->append(strError);
}

// ----------------------------------------------------------------------
void MyClient::slotSendToServer()
{
    QByteArray  arrBlock;
    QDataStream out(&arrBlock, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_10);
    out << quint16(0) << QTime::currentTime() << m_ptxtInput->text();

    out.device()->seek(0);
    out << quint16(arrBlock.size() - sizeof(quint16));

    m_pTcpSocket->write(arrBlock);
    m_ptxtInput->setText("");
}

// ------------------------------------------------------------------
void MyClient::slotConnected()
{
    m_ptxtInfo->append("Подключен к серверу.");
}

// ------------------------------------------------------------------
QString MyClient::localIP()
{
  QString locIP;
  QList<QHostAddress> addr = QNetworkInterface::allAddresses();
  locIP = addr.first().toString();
  return locIP;
}
