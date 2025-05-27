#include <iostream>
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
#include "funcs.h"
#include "path_processing.h"

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
  
  // Заполнится системой при подключении.
  sockaddr_in client_addr;
  int client_addr_len = sizeof(client_addr);
  const int buf_size_client = 1024;
  std::cout << "Waiting for a client to connect...\n";
  for (;;){
    // Блокирующая функция, которая ждет клиента. Когда клиент подключается, возвращает новый сокет(файловый дескриптор, представляющий соединение с клиентом).
    int client_socket  = accept(server_fd, (struct sockaddr *) &client_addr, (socklen_t *) &client_addr_len);
    std::thread thr([client_socket](){ // Create a new thread and run it in the background 
      short keep_alive = true;
      std::cout << "Client connected\n";
      while (keep_alive) {
        char buffer[buf_size_client] = { 0 }; 
        // Accepting user requests
        int result = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        std::stringstream response;
        std::string str_buf = std::string(buffer, result);
        if (result == 0) {
          break;
        } 

        else if (result < 0){
          std::cerr << "recv failed: " << result << "\n";
          break;
        }
        else {
          switch (choose_path(str_buf)) {
            // Normal base path
            case paths::base: {
              base_path(response, client_socket);
              break;
            }

            // path localhost:4221/echo/abc
            case paths::echo:{
              echo_path(response, client_socket, str_buf);
              break;
            }

            // path localhost:4221/user-agent
            case paths::agent:{
              agent_path(response, client_socket, str_buf);
              break;
            }

            // path like localhost:4221/files/{path_to_file}
            case paths::file: {
              file_path(response, client_socket, str_buf);
              break;
            }
            // Bad path
            case paths::def: {
              bad_path(response, client_socket);
            }
          }
        }
        if (str_buf.find("Connection: close") != std::string::npos || str_buf.find("Connection:close") != std::string::npos){
          keep_alive = false;
        }
      }
      close(client_socket);
      std::cout << "Connection closed...\n";
    });
    thr.detach(); // Main programm dont stop, this thread is running in parallel
  }
  close(server_fd);
  return 0;
}
