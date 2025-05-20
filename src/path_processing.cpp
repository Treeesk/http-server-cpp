#include  <iostream>
#include <sstream>
#include <sys/socket.h>
#include <fstream>
#include  "funcs.h"

using namespace std;
void base_path(stringstream& response, const int& client_socket) {
    response << "HTTP/1.1 200 OK\r\n"
             << "Content-Length: 0\r\n\r\n";
    send(client_socket, response.str().c_str(), response.str().size(), 0); //Send response to client
}

void echo_path(stringstream& response, const int& client_socket, string& str_buf) {
    std::string compres = check_compres(str_buf);
    if (!compres.empty()){
        std::string compressed_body;
        try {
            compressed_body = gzipCompress(str_buf.substr(str_buf.find("/echo/") + 6, str_buf.find("HTTP/1.1") - str_buf.find("/echo/") - 7));
        } catch (const std::exception& e) {
            std::cerr << "Error compress: " << e.what() << std::endl;
            response << "HTTP/1.1 500 Internal Server Error\r\n"
                     << "Content-Length: 0\r\n\r\n";
            send(client_socket, response.str().c_str(), response.str().size(), 0);
            return;
        }
        std::string status = "HTTP/1.1 200 OK\r\n";
        std::string res {
            status +
            "Content-Encoding: gzip\r\n" +
            "Content-Type: text/plain\r\n" +
            "Content-Length: " + std::to_string(compressed_body.size()) + "\r\n\r\n" + compressed_body
          };
        send(client_socket, res.data(), res.size(), 0);
        return;
    }
    else {
        response << "HTTP/1.1 200 OK\r\n" // Status line
              << "Content-Type: text/plain\r\n" // Headers
              << "Content-Length: " << 3 << "\r\n\r\n"
              << "abc"; // Body
    }
    send(client_socket, response.str().c_str(), response.str().size(), 0);
    return;
}

void agent_path(stringstream& response, const int& client_socket, string& str_buf) {
    int pos = str_buf.rfind("User-Agent:") + 11;
    if (str_buf[pos] == ' ')
        pos++;
    std::string body = "";
    while (!isspace(str_buf[pos])) {
        body += str_buf[pos];
        pos++;
    }
    response << "HTTP/1.1 200 OK\r\n" // Status line
            << "Content-Type: text/plain\r\n" // Headers
            << "Content-Length: " << body.size() << "\r\n\r\n"
            << body; // Body
    send(client_socket, response.str().c_str(), response.str().size(), 0);
}

void file_path(stringstream& response, const int& client_socket, string& str_buf) {
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
            response << "HTTP/1.1 404 Not Found\r\n"
                     << "Content-Length: 0\r\n\r\n";
        }
    }
    // POST request
    else {
        std::ofstream out(path);
        std::string request_body = str_buf.substr(str_buf.rfind("\r\n\r\n") + 4);
        out << request_body;
        response << "HTTP/1.1 201 Created\r\n"
                 << "Content-Length: 0\r\n\r\n";
        out.close();
    }
    send(client_socket, response.str().c_str(), response.str().size(), 0);
}

void bad_path(stringstream& response, const int& client_socket) {
    response << "HTTP/1.1 404 Not Found\r\n"
    << "Content-Length: 0\r\n\r\n";
    send(client_socket, response.str().c_str(), response.str().size(), 0); //Send response to client
}