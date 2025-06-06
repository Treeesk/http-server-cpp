cmake_minimum_required(VERSION 3.13)

project(http-server-starter-cpp)

file(GLOB_RECURSE SOURCE_FILES src/*.cpp src/*.hpp)

set(CMAKE_CXX_STANDARD 23) # Enable the C++23 standard
set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(Threads REQUIRED)
find_package(ZLIB REQUIRED)

add_executable(server ${SOURCE_FILES}
        src/functions.cpp
        src/funcs.h
        src/path_processing.cpp
        src/path_processing.h
        src/Threadpoll.h
        src/Threadpol_realize.cpp)

target_link_libraries(server PRIVATE Threads::Threads ZLIB::ZLIB)
