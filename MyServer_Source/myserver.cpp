#include "myserver.h"
#include "ui_myserver.h"

#include <QtNetwork>
#include <QtGui>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QList>
#include <QFile>
#include <QQueue>
#include <QFileDialog>

#include <QWidget>
#include <QDesktopWidget>
#include <QTableWidget>
#include <QToolBar>
#include <QSplitter>
#include <QLayout>
#include <QMainWindow>

QList<QTcpSocket*> clientList; //Список подключенных клиентов
int itask = 0;

QDateTime start_time;
QDateTime finish_time;
QTime t;

QQueue<QString> queueTask; // Очередь задач
int countTask;
bool flag;

QTableWidget *tableWidget;



MyServer::MyServer(int nPort, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MyServer),
    m_nNextBlockSize(0)
{
    ui->setupUi(this);

    m_ptcpServer = new QTcpServer(this);
    if (!m_ptcpServer->listen(QHostAddress::Any, nPort)) {
        QMessageBox::critical(0,
                              "Server Error",
                              "Unable to start the server:"
                              + m_ptcpServer->errorString()
                             );
        m_ptcpServer->close();
        return;
    }
    connect(m_ptcpServer, SIGNAL(newConnection()),
            this,         SLOT(slotNewConnection())
           );

    m_ptxt = new QTextEdit;
    m_ptxt->setReadOnly(true);
    m_ptxtInput = new QLineEdit;

    QPushButton* pcmd = new QPushButton("Send");
    connect(pcmd, SIGNAL(clicked()), SLOT(slotSendToClient()));

    QPushButton* fopen = new QPushButton("Открыть файл с задачами");
    connect(fopen, SIGNAL(clicked()), SLOT(readFile()));

    connect(m_ptxtInput, SIGNAL(returnPressed()), this, SLOT(slotSendToClient()));

    QPushButton* jstart = new QPushButton("Запуск");
    connect(jstart, SIGNAL(clicked()), SLOT(startJob()));


    //Размещение компонентов
    /*
    QVBoxLayout* pvbxLayout = new QVBoxLayout;
    pvbxLayout->addWidget(new QLabel("<H1>Server</H1>"));
    pvbxLayout->addWidget(m_ptxt);
    pvbxLayout->addWidget(m_ptxtInput);

    pvbxLayout->addWidget(pcmd);
    pvbxLayout->addWidget(fopen);
    pvbxLayout->addWidget(jstart);
    */

    tableWidget = new QTableWidget(); // таблица

    tableWidget->setSortingEnabled(1);
    tableWidget->setShowGrid(0);
    tableWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    tableWidget->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);


    QToolBar* toolbar = new QToolBar("Linker ToolBar"); // панель управления
    toolbar->setFixedHeight(35);
    toolbar->addWidget(fopen);
    toolbar->addWidget(jstart);

    QSplitter *spl1 = new QSplitter(Qt::Vertical, this); // Создаем первый разделитель
    setCentralWidget(spl1);
    //QVBoxLayout* Layout = new QVBoxLayout;
    //Layout->addWidget(spl1);
    //parent->setLayout(Layout);

    spl1->addWidget(toolbar);

    QSplitter *spl2 = new QSplitter(Qt::Horizontal, spl1);

    spl2->addWidget(tableWidget);
    spl2->addWidget(m_ptxt);

    spl1->setStretchFactor(spl1->indexOf(toolbar), 0); // Задаем параметры растяжения
    spl2->setStretchFactor(spl2->indexOf(tableWidget), 0); // Задаем параметры растяжения
    spl2->setStretchFactor(spl2->indexOf(m_ptxt), 0); // Задаем параметры растяжения



    QStringList lst;

    lst << "#" << "Подзадача" << "Статус" << "Время решения (ms)" << "Старт решения" << "Конец решения" << "IP";

    tableWidget->setColumnCount(lst.size());
    tableWidget->setHorizontalHeaderLabels(lst);
    tableWidget->setSortingEnabled(true);
    tableWidget->horizontalHeader()->resizeSection(0, 20);
    tableWidget->horizontalHeader()->resizeSection(1, 200);
    tableWidget->horizontalHeader()->resizeSection(3, 150);
    tableWidget->horizontalHeader()->resizeSection(6, 150);

    tableWidget->setMinimumWidth(500);
    //tableWidget->setMaximumWidth(800);

    m_ptxt->setMinimumWidth(200);
    m_ptxt->setMaximumWidth(300);

    this->setMinimumWidth(1000);

}

MyServer::~MyServer()
{
    delete ui;
}

// ----------------------------------------------------------------------
void MyServer::slotNewConnection() //Слот нового соединения
{
    QTcpSocket* pClientSocket = m_ptcpServer->nextPendingConnection();

    connect(pClientSocket, SIGNAL(disconnected()),
            pClientSocket, SLOT(deleteLater())
           );
    connect(pClientSocket, SIGNAL(readyRead()),
            this,          SLOT(slotReadClient())
           );

    sendToClient(pClientSocket, "Server: Connected!"); //Ответ сервера новому клиенту
    clientList.append(pClientSocket); //Добавляем новый клиент в список
}

// ----------------------------------------------------------------------
void MyServer::slotReadClient()
{
    QTcpSocket* pClientSocket = (QTcpSocket*)sender();

    QDataStream in(pClientSocket);
    in.setVersion(QDataStream::Qt_5_10); //::Qt_4_5

    for (;;) {
        if (!m_nNextBlockSize) {
            if (pClientSocket->bytesAvailable() < sizeof(quint16)) {
                break;
            }
            in >> m_nNextBlockSize;
        }

        if (pClientSocket->bytesAvailable() < m_nNextBlockSize) {
            break;
        }

        QTime   time;
        QString str;
        in >> time >> str;

        QString strMessage = str;

        m_ptxt->append(strMessage);

        m_nNextBlockSize = 0;

        // Проверяем завершена ли задача на клиенте
        if (strMessage.contains("endTask", Qt::CaseSensitive)) {
            // Если в очереди есть еще задачи, отсылаем их
            QString tempS = strMessage.replace("Client: endTask: ", "");
            QStringList STime = tempS.split('|');
            //lst << "#" << "Подзадача" << "Статус" << "Время решения" << "Старт решения" << "Конец решения" << "ЭВМ";

            QTableWidgetItem* ptwi = 0;

            ptwi = new QTableWidgetItem();
            ptwi->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
            ptwi->setText("Выполнена");
            tableWidget->setItem((STime.at(0)).toInt(), 2, ptwi);

            ptwi = new QTableWidgetItem();
            ptwi->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
            ptwi->setText(STime.at(3));
            tableWidget->setItem((STime.at(0)).toInt(), 3, ptwi);

            ptwi = new QTableWidgetItem();
            ptwi->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
            ptwi->setText(STime.at(1));
            tableWidget->setItem((STime.at(0)).toInt(), 4, ptwi);

            ptwi = new QTableWidgetItem();
            ptwi->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
            ptwi->setText(STime.at(2));
            tableWidget->setItem((STime.at(0)).toInt(), 5, ptwi);

            ptwi = new QTableWidgetItem();
            ptwi->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
            ptwi->setText(STime.at(4));
            tableWidget->setItem((STime.at(0)).toInt(), 6, ptwi);


            if (!queueTask.isEmpty()) {
                sendTask(pClientSocket);
            }
        }

    }
}

// ----------------------------------------------------------------------
// Отправка сообщения клиенту
void MyServer::sendToClient(QTcpSocket* pSocket, const QString& str)
{
    QByteArray  arrBlock;
    QDataStream out(&arrBlock, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_10);
    out << quint16(0) << QTime::currentTime() << str;

    out.device()->seek(0);
    out << quint16(arrBlock.size() - sizeof(quint16));

    pSocket->write(arrBlock);
}


// ----------------------------------------------------------------------
// Отправка сообщения с роля Input
void MyServer::slotSendToClient()
{
    foreach (QTcpSocket* pClientSocket, clientList) {
        sendToClient(pClientSocket, m_ptxtInput->text());
    }

    m_ptxtInput->setText("");
}

// ----------------------------------------------------------------------
// Чтение файла
void MyServer::readFile()
{
    QString fileName = QFileDialog::getOpenFileName(0, "Open Dialog", "", "");

    QFile file(fileName);

    m_ptxt->clear();

    int i = 0;

    if(file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        m_ptxt->append("Task list:");
        while(!file.atEnd()) {
            QString str = file.readLine();
            str.replace("\n", "");
            m_ptxt->append("\t" + QString::number(i) + ": " + str);
            queueTask.enqueue(str);
            addItem(i, str);
            i++;
        }
        countTask = i;
        flag = 1;
        m_ptxt->append("end task list;");
        m_ptxt->append("");
    }
    else
    {
        m_ptxt->append("don't open file");
    }
    file.close();

}

// ----------------------------------------------------------------------
// Добавление в таблицу
void MyServer::addItem(int i, QString str)
{
    QTableWidgetItem *ptwi = 0;

    tableWidget->setRowCount(i + 1);

    //lst << "#" << "Подзадача" << "Статус" << "Время решения" << "Старт решения" << "Конец решения" << "ЭВМ";

    ptwi = new QTableWidgetItem();
    ptwi->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
    ptwi->setText(QString::number(i));
    tableWidget->setItem(i, 0, ptwi);

    ptwi = new QTableWidgetItem();
    ptwi->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
    ptwi->setText(str);
    tableWidget->setItem(i, 1, ptwi);

    ptwi = new QTableWidgetItem();
    ptwi->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
    ptwi->setText("-");
    tableWidget->setItem(i, 2, ptwi);

    ptwi = new QTableWidgetItem();
    ptwi->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
    ptwi->setText("-");
    tableWidget->setItem(i, 3, ptwi);

    ptwi = new QTableWidgetItem();
    ptwi->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
    ptwi->setText("-");
    tableWidget->setItem(i, 4, ptwi);

    ptwi = new QTableWidgetItem();
    ptwi->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
    ptwi->setText("-");
    tableWidget->setItem(i, 5, ptwi);

    ptwi = new QTableWidgetItem();
    ptwi->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
    ptwi->setText("-");
    tableWidget->setItem(i, 6, ptwi);

}

// ----------------------------------------------------------------------
// Отправка задачи задач
void MyServer::sendTask(QTcpSocket* pClientSocket)
{


    QString currentTask;

    currentTask = queueTask.dequeue();

    m_ptxt->append("\nSend task " +
                   QString::number(itask + 1) + "/" +
                   QString::number(countTask) + " : " +
                   currentTask
                   );

    sendToClient(pClientSocket, QString::number(itask) + "--" + currentTask);

    itask++;

    if (queueTask.isEmpty()) {
        m_ptxt->append("tasks ended");

        finish_time = QDateTime::currentDateTime();
        int el = t.elapsed();

        // Выводим информацию о времени расчетов
        m_ptxt->append("start: " + start_time.toString("hh:mm:ss"));
        m_ptxt->append("finish: " + finish_time.toString("hh:mm:ss"));
        m_ptxt->append("delay: " + QString::number(el / 1000) + "s");
    }

}

// ----------------------------------------------------------------------
// Запуск задач
void MyServer::startJob()
{
    itask = 0;

    start_time = QDateTime::currentDateTime();
    t.start();

    foreach (QTcpSocket* pClientSocket, clientList) {
        sendTask(pClientSocket);
    }
}
