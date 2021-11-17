#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->stop->setEnabled(false);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_start_clicked()
{
    tcpServer = new QTcpServer(this);
    connect(tcpServer, SIGNAL(newConnection()), this, SLOT(newuser()));

    if (!tcpServer->listen(QHostAddress::Any, 728) && server_status==0) {
        qDebug() <<  tcpServer->errorString();
        ui->textinfo->append(tcpServer->errorString());
    } else {
        server_status=1;
        qDebug() << "Сервер запущен. Порт 728";
        ui->textinfo->append(QString::fromUtf8("Сервер запущен. Порт 728"));
        qDebug() << QString::fromUtf8("Сервер запущен");

        ui->start->setEnabled(false);
        ui->stop->setEnabled(true);
    }
}

void MainWindow::on_stop_clicked()
{
    if(server_status==1){
        foreach(int i,clients.keys()){
            QTextStream os(clients[i]);
            os.setAutoDetectUnicode(true);
            os << QDateTime::currentDateTime().toString() << "\n";
            clients[i]->close();
            clients.remove(i);
        }
        tcpServer->close();
        ui->textinfo->append(QString::fromUtf8("Сервер остановлен"));
        qDebug() << QString::fromUtf8("Сервер остановлен");
        server_status=0;
        ui->start->setEnabled(true);
        ui->stop->setEnabled(false);
    }
}


void MainWindow::newuser()
{
    if(server_status==1){

        QTcpSocket* clientSocket=tcpServer->nextPendingConnection();

        qDebug() << clientSocket->socketDescriptor() << " - новое соединение";
        ui->textinfo->append(QString::number(clientSocket->socketDescriptor()));
        ui->textinfo->append(QString::fromUtf8("Новое соединение"));

        clients[clientSocket->socketDescriptor()]=clientSocket;

        connect(clients[clientSocket->socketDescriptor()],SIGNAL(readyRead()),this, SLOT(slotReadClient()));
        connect(clients[clientSocket->socketDescriptor()],SIGNAL(disconnected()),this, SLOT(slotDisconnected()));
    }
}

void MainWindow::slotReadClient()
{
    QTcpSocket* clientSocket = (QTcpSocket*)sender();
    QByteArray Data = clientSocket->readAll();
    QString a = QString(Data[0]);
    int ch = a.toInt();

    switch (ch)
    {
    case 0:
        {
            Data.remove(0,1);

            users[Data] = clientSocket;

            break;
        };
    case 1:
        {
            Data.remove(0,1);

            if ( pm.find(clientSocket) != pm.end() )
            {
               clientSocket = pm[clientSocket];
               QTextStream os(clientSocket);
               os << Data;
            }
            else
            {
                QTextStream os(clientSocket);
                os << "Пользователь не найден";
            }
            break;
        };
    case 2:
        {
        Data.remove(0,1);
        QMap<QString,QTcpSocket*>::const_iterator i;
        for(i = users.constBegin();i != users.constEnd(); i++)
        {
             if(i.key() == Data)
             {
                 QString msg = "Пользователь "+ users.key(clientSocket)+ " хочет вам написать";

                 pm[clientSocket] = i.value();
                 clientSocket = i.value();

                 QTextStream os(clientSocket);
                 os.setAutoDetectUnicode(true);
                 os << msg;
             }

        }
            break;
        };
    default:
        break;
    }
}

void MainWindow::slotDisconnected()
{
    QTcpSocket* clientSocket = (QTcpSocket*)sender();

    clients.remove(clientSocket->socketDescriptor());
    if (users.find(users.key(clientSocket)) != users.end())
    {
        users.remove(users.key(clientSocket));
    }
    if (pm.find(clientSocket) != pm.end())
    {
        pm.remove(clientSocket);
    }
    if (pm.find(pm.key(clientSocket)) != pm.end())
    {
        pm.remove(pm.key(clientSocket));
    }

    qDebug() << clientSocket->socketDescriptor() << " - отключен";
    ui->textinfo->append(QString::number(clientSocket->socketDescriptor()));
    ui->textinfo->append("Отключен");
    clientSocket->deleteLater();
}

