#include <cstdlib>
#include <fstream>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdbool.h>
#include <thread>
#include <vector>
#include <poll.h>
#include "funcs.h"
#include "path_processing.h"
#include "Threadpoll.h"

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
  int connection_backlog = 100;
  if (listen(server_fd, connection_backlog) != 0) {
    std::cerr << "listen failed\n";
    return 1;
  }
  sockaddr_in client_addr;
  int client_addr_len = sizeof(client_addr);
  std::cout << "Waiting for a client to connect...\n";
  // int pipefd[2]; // "Труба" для двухсторонней связи между потоками, [0] - читает, [1] - пишет. В первый элемент будем писать в не основном потоке, а в основном из нулевого элемента будет читать (значит данные появились)
  // if (pipe(pipefd)) {
  //   std::cerr << "ERROR: Failed pipe!\n";
  // }
  // // poll позволяет следить за всеми сокетами и как только клиентский поток готов к записи, только тогда создает новый поток(новый поток не создается сразу после accept).
  std::vector<pollfd> pfds(1);
  // pfds[0].fd = pipefd[0];
  // pfds[0].events = POLLIN;
  pfds[0].fd = server_fd; // Accept poll
  pfds[0].events = POLLIN;
  int client_socket;
  ThreadPoll thread_poll(8); // initialization 8 thread.
  thread_poll.start();
  for (;;){
    struct timeval timeout; // Время ожидания на отправку повторого запроса в recv одним и тем же соединением.
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    int rv = poll(pfds.data(), pfds.size(), 60000); // 60 sec timeout 
    if (rv == -1) {
      std::cerr << "POLL ERROR\n";
      continue;
    }
    else if (rv == 0) {
      std::cerr << "Timeout occurred! No data after 60 seconds.\n";
      continue;
    }
    else {
      for (int i = 0; i < pfds.size(); ++i){
        if (pfds[i].revents & (POLLERR | POLLHUP)){
            std::cerr << "Socket " << pfds[i].fd << " has error or hangup. Removing.\n";
            close(pfds[i].fd);
            pfds.erase(pfds.begin() + i);
            --i;
            continue;
        }
        else if (pfds[i].revents & POLLIN) {
          if (pfds[i].fd == server_fd) {
            client_socket = accept(server_fd, (struct sockaddr *) &client_addr, (socklen_t *) &client_addr_len);
            setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
            pfds.push_back({client_socket, POLLIN, 0});
          }
          else {
            int client_socket = pfds[i].fd;
            thread_poll.add_task(client_socket);
            pfds.erase(pfds.begin() + i);
            --i;
          }
        }
      }
    }
  }
  close(server_fd);
  return 0;
}
