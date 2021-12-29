FROM gcc:latest

COPY . /usr/src/server

WORKDIR /usr/src/server

RUN g++ -o untitled4 main.cpp

CMD ["./untitled4"]