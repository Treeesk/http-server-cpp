#include <iostream>
#include "funcs.h"
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
