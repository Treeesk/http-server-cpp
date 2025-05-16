#include <iostream>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "funcs.h"

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
    std::cout << "Client connected\n";
    char buffer[buf_size_client] = { 0 };
    // Accepting user requests
    int result = recv(client_socket, buffer, sizeof(buffer), 0);
    std::stringstream response;
    if (result == 0) {
      std::cerr << "Connection closed...\n";
    }

    else if (result < 0){
      std::cerr << "recv failed: " << result << "\n";
      close(client_socket);
    }
    
    else {
      std::string str_buf = std::string(buffer);
      switch (choose_path(str_buf)) {
        // Normal base path
        case paths::base: {
          response << "HTTP/1.1 200 OK\r\n";
          send(client_socket, response.str().c_str(), response.str().size(), 0); //Send response to client
          break;
        }

        // path localhost:4221/echo/abc
        case paths::echo:{
          std::string compres = check_compres(str_buf);
          if (compres != ""){
            response << "HTTP/1.1 200 OK\r\n" // Status line
                    << "Content-Type: text/plain\r\n" // Headers
                    << "Content-Encoding: " << compres << "\r\n"
                    << "Content-Length: " << 3 << "\r\n\r\n"
                    << "abc"; // Body
          }
          else {
              response << "HTTP/1.1 200 OK\r\n" // Status line
                    << "Content-Type: text/plain\r\n" // Headers
                    << "Content-Length: " << 3 << "\r\n\r\n"
                    << "abc"; // Body
          }
          send(client_socket, response.str().c_str(), response.str().size(), 0);
          break;
        }

        // path localhost:4221/user-agent
        case paths::agent:{
          int pos = str_buf.rfind("User-Agent:") + 11;
          if (str_buf[pos] == ' ')
            pos++;
          std::string body = str_buf.substr(pos);
          response << "HTTP/1.1 200 OK\r\n" // Status line
                  << "Content-Type: text/plain\r\n" // Headers
                  << "Content-Length: " << body.size() << "\r\n\r\n"
                  << body; // Body
          send(client_socket, response.str().c_str(), response.str().size(), 0);
          break;
        }

        // path like localhost:4221/files/{path_to_file}
        case paths::file: {
          std::string path = "/tmp/" + str_buf.substr(str_buf.find("/files/") + 7, str_buf.find("HTTP/1.1") - str_buf.find("/files/") - 8);
          // GET request
          if (parse_command(str_buf) == "GET"){
            std::ifstream in(path);
            if (in.is_open()){
              std::stringstream body;
              body << in.rdbuf(); // in.rdbuf() return adress buf file, body << (function take adress and read buffer file)
              std::string st = body.str();
              response << "HTTP/1.1 200 OK\r\n" // Status line
                      << "Content-Type: application/octet-stream\r\n" // Headers
                      << "Content-Length: " << st.size() << "\r\n\r\n"
                      << st; // body
              in.close();
            }
            else {
              response << "HTTP/1.1 404 Not Found\r\n\r\n";
            }
          }
          // POST request
          else {
            std::ofstream out(path);
            std::string request_body = str_buf.substr(str_buf.rfind("\r\n\r\n") + 4);
            out << request_body;
            response << "HTTP/1.1 201 Created\r\n\r\n";
            out.close();
          }
          send(client_socket, response.str().c_str(), response.str().size(), 0);
          break;
        }
        // Bad path
        case paths::def: {
          response << "HTTP/1.1 404 Not Found\r\n\r\n";
          send(client_socket, response.str().c_str(), response.str().size(), 0); //Send response to client
        }
      }
      close(client_socket);
      std::cout << "Connection closed...\n";
    }
  }
  close(server_fd);
  return 0;
}
