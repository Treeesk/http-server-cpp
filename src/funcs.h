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
paths choose_path(std::string& str);
#endif //FUNCS_H
