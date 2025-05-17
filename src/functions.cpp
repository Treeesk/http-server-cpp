#include <iostream>
#include "funcs.h"
#include <zlib.h>
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