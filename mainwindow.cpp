#include "mainwindow.h"
#include "ui_mainwindow.h"

QTextStream & operator<< (QTextStream &stream, QVector<personal_msg> a)
{
    stream << "2";
    for (int i = 0; i < a.size(); i++)
    {
        stream << a[i].sender.size()
        << "/" << a[i].time.size()
        << "/" << a[i].msg.size()
        << "/" << a[i].sender + a[i].time + a[i].msg;
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
    if(!QFile::exists("db.db"))
    {
        db.setDatabaseName("db.db");
        db.open();

        QSqlQuery q;

        q.exec("CREATE DATABASE IF NOT EXISTS db;");
        q.exec("CREATE TABLE IF NOT EXISTS pm ("
                   "id	INTEGER NOT NULL,"
                   "sender	INTEGER NOT NULL,"
                   "reciever	INTEGER NOT NULL,"
                   "msg	TEXT NOT NULL,"
                   "time	TEXT,"
                   "CONSTRAINT FK__users_2 FOREIGN KEY(reciever) REFERENCES users(id) ON UPDATE NO ACTION ON DELETE NO ACTION,"
                   "CONSTRAINT FK__users FOREIGN KEY(sender) REFERENCES users(id) ON UPDATE NO ACTION ON DELETE NO ACTION,"
                   "PRIMARY KEY(id AUTOINCREMENT)"
                   ");");
        q.exec("CREATE TABLE IF NOT EXISTS users ("
                   "id INTEGER NOT NULL,"
                   "login TEXT NOT NULL UNIQUE,"
                   "pass	TEXT NOT NULL,"
                   "PRIMARY KEY(id AUTOINCREMENT)"
                   ");");
    }
    else
    {
        db.setDatabaseName("db.db");
        db.open();
    }

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

    connect(clients[clientSocket->socketDescriptor()],SIGNAL(disconnected()),this,SLOT(slotDisconnected()));
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

                if(users.find(QString(log)) != users.end()) // user online or nah
                {
                    data.insert(0,"02");
                }
                else
                {
                    users[log] = clientSocket;
                    data.insert(0,"01");
                }

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
            Data.remove(0,1);
            QString msg = QString(Data);
            QSqlQuery q;
            QString reciever = msg.mid(0,msg.indexOf("/"));
            QString sender = users.key(clientSocket);

            if (is_user_exist(msg.mid(0,msg.indexOf("/"))) == 1)
            {
                msg.remove(0,msg.indexOf("/")+1);

                q.prepare("INSERT INTO pm (sender, reciever, msg, TIME) VALUES ((SELECT id FROM users WHERE login = :user1),(SELECT id FROM users WHERE login = :user2),:msg, :time)");
                q.bindValue(":user1",sender);//ник отправителя
                q.bindValue(":user2",reciever);//ник получателя
                q.bindValue(":msg",msg);
                q.bindValue(":time",QDateTime::currentDateTimeUtc().toString());
                q.exec();

                if(users.find(reciever) != users.end())
                {
                    clientSocket = users[reciever];
                    QTextStream os(clientSocket);
                    QString all = QString::number(sender.size());
                    all.push_back(sender);
                    all.insert(0,"1/");
                    all.push_back(msg);
                    os << all;
                }
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

        QSqlQuery query;

        if(is_user_exist(QString(Data)) == 1)
        {
            QVector<personal_msg> a;
            personal_msg temp;

            query.prepare("SELECT t.* FROM (SELECT pm.id, login, msg, time FROM pm JOIN users ON users.id = pm.sender WHERE (sender = (SELECT id FROM users WHERE login = :user1) and reciever = (SELECT id FROM users WHERE login = :user2)) or (sender = (SELECT id FROM users WHERE login = :user2) and reciever = (SELECT id FROM users WHERE login = :user1))ORDER BY pm.id DESC LIMIT 50) AS t ORDER BY t.id asc ");//страшна вырубай
            query.bindValue(":user1",users.key(clientSocket));
            query.bindValue(":user2",QString(Data));
            query.exec();

            while (query.next())
            {
                temp.sender = query.value(1).toString();
                temp.msg = query.value(2).toString();
                temp.time = query.value(3).toString();
                a.push_back(temp);
            }

            QTextStream os(clientSocket);
            os << a;
        }
        else
        {
            QTextStream os(clientSocket);
            os << "5Пользователь не найден";
        }

        break;
        };
    case 3://disconnect because signal doesn't work dunno why
        {
            /*qDebug() << clientSocket->socketDescriptor();

            clients.remove(clientSocket->socketDescriptor());
            if (users.find(users.key(clientSocket)) != users.end())
            {
                users.remove(users.key(clientSocket));
            }

            qDebug() << clientSocket->socketDescriptor() << " - отключен";
            ui->textinfo->append(QString::number(clientSocket->socketDescriptor()) + " Отключен");
            clientSocket->deleteLater();*/

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
                os << "01";
            }
            else
            {
                QTextStream os(clientSocket);
                os << "02";
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
    ui->textinfo->append("Отключен");

    clients.remove(clients.key(clientSocket));
    if (users.find(users.key(clientSocket)) != users.end())
    {
        users.remove(users.key(clientSocket));
    }
    clientSocket->deleteLater();
}

int MainWindow::is_user_exist(QString Data)
{
    QSqlQuery query;
    int existance = 0;

    query.prepare("SELECT COUNT(*) FROM users WHERE login = :user");
    query.bindValue(":user",Data);
    query.exec();
    while (query.next())
    {
        existance = query.value(0).toInt();
    }
    return existance;
}


