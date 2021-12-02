#include "mainwindow.h"
#include "ui_mainwindow.h"


QTextStream & operator<< (QTextStream &stream, QVector<personal_msg> a)
{
    stream << "2";
    for (int i = 0; i < a.size(); i++)
    {
        stream << a[i].sender + "/" + a[i].time + "/" + a[i].reciever + "/" + a[i].msg + "]";
    }

    return stream;
}


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->stop->setEnabled(false);

    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("db.db");
    /*
    CREATE TABLE IF NOT EXISTS "users" (
        "id"	INTEGER NOT NULL,
        "login"	TEXT NOT NULL UNIQUE,
        "pass"	TEXT NOT NULL,
        PRIMARY KEY("id" AUTOINCREMENT)
    );
    */
    qDebug() << db.open();

}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_start_clicked()
{
    tcpServer = new QTcpServer(this);
    connect(tcpServer, SIGNAL(newConnection()), this, SLOT(newuser()));

    if (!tcpServer->listen(QHostAddress::Any, 728))
    {
        qDebug() <<  tcpServer->errorString();
        ui->textinfo->append(tcpServer->errorString());
    }
    else
    {
        qDebug() << "Сервер запущен. Порт 728";
        ui->textinfo->append(QString::fromUtf8("Сервер запущен. Порт 728"));
        qDebug() << QString::fromUtf8("Сервер запущен");

        ui->start->setEnabled(false);
        ui->stop->setEnabled(true);
    }
}

void MainWindow::on_stop_clicked()
{

    foreach(int i,clients.keys())
    {
        QTextStream os(clients[i]);
        os.setAutoDetectUnicode(true);
        os << QDateTime::currentDateTime().toString() << "\n";
        clients[i]->close();
        clients.remove(i);
    }

    tcpServer->close();
    ui->textinfo->append(QString::fromUtf8("Сервер остановлен"));
    qDebug() << QString::fromUtf8("Сервер остановлен");

    ui->start->setEnabled(true);
    ui->stop->setEnabled(false);

}

void MainWindow::newuser()
{
    QTcpSocket* clientSocket=tcpServer->nextPendingConnection();

    qDebug() << clientSocket->socketDescriptor() << " - новое соединение";
    ui->textinfo->append(QString::number(clientSocket->socketDescriptor())+ " Новое соединение");

    clients[clientSocket->socketDescriptor()]=clientSocket;

    connect(clients[clientSocket->socketDescriptor()],SIGNAL(readyRead()),this, SLOT(slotReadClient()));
//  сonnect(clients[clientSocket->socketDescriptor()],SIGNAL(disconnected()),this, SLOT(slotDisconnected()));
}

void MainWindow::slotReadClient()
{
    QTcpSocket* clientSocket = (QTcpSocket*)sender();
    QByteArray Data = clientSocket->readAll();
    QString a = QString(Data[0]);
    int ch = a.toInt();

    switch (ch)
    {
    case 0://log in
        {
            Data.remove(0,1);

            int value = 0;
            QSqlQuery q;
            QString log = QString(Data);
            QString pass = QString(Data);

            log.remove(log.indexOf("+"),log.size()-log.indexOf("+"));
            pass.remove(0,pass.indexOf("+")+1);

            q.exec("SELECT COUNT(*) FROM users WHERE login ='"+log+"' AND pass='"+pass+"';");
            while (q.next())
            {
                value = q.value(0).toInt();
            }
            if(value == 1)
            {
                QByteArray data;
                data.insert(0,"01");

                users[log] = clientSocket;

                QTextStream os(clientSocket);
                os << data;
            }
            else
            {
                QTextStream os(clientSocket);
                QByteArray data;

                data.insert(0,"00");
                os << data;
            }
            break;
        };
    case 1: //send to another user
        {
            //Data.remove(0,1);

            if ( pm.find(clientSocket) != pm.end() )
            {
                QString msg = QString(Data);
                msg.remove(0,1);
                QSqlQuery q;
                q.prepare("INSERT INTO pm (sender, reciever, msg, TIME) VALUES ((SELECT id FROM users WHERE login = :user1),(SELECT id FROM users WHERE login = :user2),:msg, :time)");
                q.bindValue(":user1",users.key(clientSocket));
                q.bindValue(":user2",users.key(pm[clientSocket]));//самое главное потом разобарться хахаха
                q.bindValue(":msg",msg);
                q.bindValue(":time",QDateTime::currentDateTimeUtc().toString());
                q.exec();

               clientSocket = pm[clientSocket];
               QTextStream os(clientSocket);
               os << Data;
            }
            else
            {
                qDebug() << clientSocket->socketDescriptor();
                QTextStream os(clientSocket);
                os << "1Пользователь не найден";
            }
            break;
        };
    case 2://find a user
        {
        Data.remove(0,1);


        if(users.find(Data) != users.end())
        {
            //QString msg = "Пользователь "+ users.key(clientSocket)+ " хочет вам написать";

            QSqlQuery q;

            QVector<personal_msg> a;
            personal_msg temp;

            q.prepare("SELECT login, reciever, msg, time FROM pm JOIN users ON users.id = pm.sender WHERE (sender = (SELECT id FROM users WHERE login = :user1) and reciever = (SELECT id FROM users WHERE login = :user2)) or (sender = (SELECT id FROM users WHERE login = :user2) and reciever = (SELECT id FROM users WHERE login = :user1))");
            q.bindValue(":user1",users.key(clientSocket));
            q.bindValue(":user2",QString(Data));
            q.exec();

            while (q.next())
            {
                temp.sender = q.value(0).toString();
                temp.reciever = q.value(1).toString();
                temp.msg = q.value(2).toString();
                temp.time = q.value(3).toString();
                a.push_back(temp);
            }

            QTextStream da(clientSocket);
            da << a;
            pm[clientSocket] = users.value(Data);
            clientSocket = users.value(Data);

            /*QTextStream os(clientSocket);
            os.setAutoDetectUnicode(true);
            os << msg;*/


            QTextStream lol(clientSocket);
            lol << a;

        }
        else
        {
            qDebug() << clientSocket->socketDescriptor();
            QTextStream os(clientSocket);
            os << "Пользователь не найден";
        }

        break;
        };
    case 3://disconnect because signal doesn't work dunno why
        {
            qDebug() << clientSocket->socketDescriptor();

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
            ui->textinfo->append(QString::number(clientSocket->socketDescriptor()) + " Отключен");
            clientSocket->deleteLater();

            break;
        };
    case 4://registration(why 4(because i'm a genius))
        {
            Data.remove(0,1);

            QSqlQuery q;
            QString log = QString(Data);
            QString pass = QString(Data);

            log.remove(log.indexOf("+"),log.size()-log.indexOf("+"));
            pass.remove(0,pass.indexOf("+")+1);

            bool result = q.exec("INSERT INTO users ('login', 'pass') VALUES ('"+log+"', '"+pass+"');");

            if(result == true)
            {
                users[log] = clientSocket;
                QTextStream os(clientSocket);
                os << "1";
            }
            else
            {
                QTextStream os(clientSocket);
                os << "0";
            }

            break;
        };
    default:
        break;
    }
}


