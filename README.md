# TCPserver chat

## Для работы необходимо скачать сурсы Qt 6.2

> Stack

![Qt](https://img.shields.io/badge/Qt-%23217346.svg?style=for-the-badge&logo=Qt&logoColor=white)
![SQLite](https://img.shields.io/badge/sqlite-%2307405e.svg?style=for-the-badge&logo=sqlite&logoColor=white)

>Utilites

![Docker](https://img.shields.io/badge/docker-%230db7ed.svg?style=for-the-badge&logo=docker&logoColor=white)

### Запуск приложения в докере

```yml
git clone https://github.com/A1caida/tcpserver_chat.git
cd tcpserver_chat
docker build . -t server:1
docker run --rm -it server:1
```

### Подробная инструкиця

> Колнируем этот репозиторий в любую директорию и переходим в нее
```yml
git clone https://github.com/A1caida/tcpserver_chat.git
cd tcpserver_chat
```

> Далее собираем образ коммандой
```yml
docker build . -t server:1
```
![da](https://github.com/A1caida/tcpserver_chat/blob/experemental/1.jpg)

> И запускаем
```yml
docker run --rm -it server:1
```
![winda](https://github.com/A1caida/tcpserver_chat/blob/experemental/2.png)
