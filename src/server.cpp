#include <cstdlib>
#include <fstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "funcs.h"
#include "Threadpoll.h"
#include <fcntl.h>
#include <sys/event.h>

int main(int argc, char **argv) {
  // Flush after every std::cout / std::cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  // создание сокета (возвращает файловый дескриптор (указатель на сокет)) AF_INET=IPv4, sock_stream = TCP, 0-возможность OS автоматич. определить протокол для TCP
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
   std::cerr << "Failed to create server socket\n";
   return 1;
  }
  
  // Since the tester restarts your program quite often, setting SO_REUSEADDR
  // ensures that we don't run into 'Address already in use' errors
  // Настройка параметров сокета, SO_REUSEADDR=параметр, разрешающий повторно использовать адрес(если без этого, после завершения, если сразу запустить будет ошибка)
  // reuse=включить эту опцию
  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
    std::cerr << "setsockopt failed\n";
    return 1;
  }
  
  // Тип данных для хранения адреса сокета. htons() - преобразует unsigned int из порядка байтов машины в порядок байтов сети
  // INADDR_ANY: используется, когда мы не хотим привязывать наш сокет к какому-либо конкретному IP-адресу, а вместо этого заставлять его прослушивать все доступные IP-адреса(Обращаться можно по любому доступному IP адресу на машине)
  sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(4221);
  
  // Соединение созданного дексриптора с адресом сокета(т.е его Ip и порт). Это как телефон и номер, их нужно связать между собой, чтобы звонить. Привязывает сокет к адресу 0.0.0.0:4221
  if (bind(server_fd, (sockaddr *) &server_addr, sizeof(server_addr)) != 0) {
    std::cerr << "Failed to bind to port 4221\n"; 
    return 1;
  }
  
  // Затем указываем слушать сокет server_fd, чтобы он мог принимать входящие подключения через accept. Максимум 25 клиентов на подключение. 
  int connection_backlog = 25;
  if (listen(server_fd, connection_backlog) != 0) {
    std::cerr << "listen failed\n";
    return 1;
  }
  std::cout << "Waiting for a client to connect...\n";

  // Создается структура kevent, с помощью макроса EV_SET она описывается. затем kevent принимает события за которыми следишь, и заполняет массив с событиями которые произошли
  int kq = kqueue(); // Очередь в Ядре ОС, с которой происходит работа для получения информации или предоставления.
  struct kevent ev; // want monitor 
  std::vector<struct kevent> trig(64); // То на что приходят события(64 максимум за раз)
  EV_SET(&ev, server_fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
  kevent(kq, &ev, 1, NULL, 0, NULL); // регистрируем событие, ничего не ожидаем 

  struct timespec tmout{60, 0}; 

  ThreadPoll thread_poll(8, kq); // initialization 8 thread.
  thread_poll.start();
  for (;;){
    int nev = kevent(kq, NULL, 0, trig.data(), trig.size(), &tmout);
 
    if (nev == -1) {
      std::cerr << "KEVENT ERROR\n";
      continue;
    }
    else if (nev == 0) {
      std::cerr << "Timeout occurred! No data after 60 seconds.\n";
      continue;
    }
    else {
      for (int i = 0; i < nev; ++i){
        if (trig[i].flags & (EV_ERROR | EV_EOF)){
            std::cerr << "Socket " << trig[i].ident << " has error or hangup. Removing.\n";
            close(trig[i].ident); // Автоматически удалится и из kqueue, потому что ядро следит за файловыми дескрипторами
            // когда он закрывается, становится не валидным-перестает.
        }
        else {
          if (trig[i].ident == server_fd) {
            sockaddr_in client_addr;
            int client_addr_len = sizeof(client_addr);
            int client_socket = accept(server_fd, (struct sockaddr *) &client_addr, (socklen_t *) &client_addr_len);
            if (client_socket < 0) {
              if (errno == EAGAIN || errno == EWOULDBLOCK){
                continue;
              }
              else {
                std::cerr << "accepted failed\n";
                continue;
              }
            }
            int flags = fcntl(client_socket, F_GETFL, 0);
            fcntl(client_socket, F_SETFL, flags | O_NONBLOCK); // неблокирующее состояние сокета
            struct kevent client;
            EV_SET(&client, client_socket, EVFILT_READ, EV_ADD, 0, 0, NULL);
            kevent(kq, &client, 1, NULL, 0, NULL); // регистрируем событие, ничего не ожидаем
          }
          else {
            int client_socket = trig[i].ident;
            struct kevent change;
            EV_SET(&change, client_socket, EVFILT_READ, EV_DELETE, 0, 0, NULL);
            kevent(kq, &change, 1, NULL, 0, NULL); // удалить нужно сразу, т.к пока в другом потоке проходит, может снова добавиться 
            thread_poll.add_task(client_socket);
          }
        }
      }
    }
  }
  close(server_fd);
  return 0;
}
