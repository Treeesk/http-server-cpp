#ifndef FUNCS_H
#define FUNCS_H
#include <string>

enum class paths {
    base,
    echo,
    agent,
    file,
    def,
};

std::string parse_command(std::string&);
paths choose_path(std::string&);
std::string check_compres(std::string&);
std::string gzipCompress(const std::string_view);
void connection_processing(int client_socket);
#endif //FUNCS_H
