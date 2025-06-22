#ifndef THREADPOLL_H
#define THREADPOLL_H
#include <thread>
#include <vector>
#include <queue>
#include <condition_variable>
#include <mutex>
#include "funcs.h"

// Обертка для отслеживания состояния каждого потока. atomic(атомарные данные, с которыми можно взаимодействовать из разных потоков)
// Согласованное состояние переменной между потоками.
struct Thread
{
    std::thread _thread;
    std::atomic<bool> is_working;
};

class ThreadPoll {
    private:
        std::atomic<bool> stopped;
        std::atomic<bool> paused;
        std::vector<Thread*> workers;
        std::queue<int> tasks_queue; // for client_socket
        void worker_func(Thread*);
        std::mutex task_queue_mut; 
        std::condition_variable tasks_access;
    public:
        ThreadPoll(int cnt_threads);
        void start();
        void stop();
        void add_task(int client_sock);
        ~ThreadPoll();
};

#endif //THREADPOLL_H
