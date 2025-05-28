#include <sys/socket.h>
#include <zlib.h>
#include "funcs.h"
#include "path_processing.h"

// Parse method(GET, POST...)
std::string parse_command(std::string& str) {
    std::string com = "";
    int i = 0;
    while (!isspace(str[i])){
        com += str[i];
        i++;
    }
    return com;
}

paths choose_path(std::string& str) {
    int i = 0;
    while (!isspace(str[i++]));
    if (isspace(str[i + 1])) {
        return paths::base;
    }
    if (str.find("/echo/abc") != std::string::npos) {
        return paths::echo;
    }
    if (str.find("/user-agent") != std::string::npos) {
        return paths::agent;
    }
    if (str.find("/files/") != std::string::npos) {
        return paths::file;
    }
    return paths::def;
}

std::string check_compres(std::string& str)
{
    std::string compres = "";
    std::string line_comprs = str.substr(str.find("Accept-Encoding:") + 16);
    if (line_comprs.find("gzip") != std::string::npos){
        compres = "gzip";
    }
    return compres;
}


std::string gzipCompress(const std::string_view input){
    constexpr int windowBits = 15 + 16;
    constexpr int memLevel = 8;
    z_stream zStream{};
    if(deflateInit2(&zStream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, windowBits, memLevel, Z_DEFAULT_STRATEGY) !=Z_OK){
        throw std::runtime_error("deflateInit2 failed");
    }
    zStream.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(input.data()));
    zStream.avail_in = static_cast<uInt>(input.size());
    std::string compressed{};
    compressed.resize(deflateBound(&zStream, zStream.avail_in));
    zStream.next_out = reinterpret_cast<Bytef*>(&compressed[0]);
    zStream.avail_out = static_cast<uInt>(compressed.size());
    int ret = deflate(&zStream, Z_FINISH);
    if(ret != Z_STREAM_END) {
        deflateEnd(&zStream);
        throw std::runtime_error("Deflate failed");
    }
    compressed.resize(zStream.total_out);
    deflateEnd(&zStream);
    return compressed;
}


void connection_processing(int client_socket){
    short keep_alive = true;
    const int buffer_size = 1024;
    std::cout << "Client connected\n";
    while (keep_alive) {
        char buffer[buffer_size] = { 0 };
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
}