#ifndef PATH_PROCESSING_H
#define PATH_PROCESSING_H
#include <sstream>
#include <string>

void base_path(std::stringstream& response, const int& client_socket);
void echo_path(std::stringstream& response, const int& client_socket, std::string& str_buf);
void agent_path(std::stringstream& response, const int& client_socket, std::string& str_buf);
void file_path(std::stringstream& response, const int& client_socket, std::string& str_buf);
void bad_path(std::stringstream& response, const int& client_socket);
#endif //PATH_PROCESSING_H
