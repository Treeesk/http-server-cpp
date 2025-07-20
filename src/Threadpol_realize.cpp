#include "Threadpoll.h"

void ThreadPoll::worker_func(Thread* th){
    while (!stopped){
        std::unique_lock<std::mutex> lock(task_queue_mut); // нужно захватить мьютекс. Если сделать это не удаётся, происходит блокировка, которая продолжается до тех пор, пока мьютекс не будет захвачен. Мьютекс захватывается во избежание критических состязаний при пробуждении.
        th->is_working = false;
        tasks_access.wait(lock, [this]()->bool{return ((!tasks_queue.empty() && !paused) || stopped);}); // Ждать если нет задач в очереди либо работа пула приостановлена
        // lock в wait нужен, если происходит notify_all или notify_one, то tasks_access пробуждается и проверяется функтор, если выдается false, то происходит lock.unlock() и tasks_access засыпает до следующего пробуждения.
        th->is_working = true;
        if (!tasks_queue.empty()){
            auto elem = tasks_queue.front(); // Take task from queue
            tasks_queue.pop();
            lock.unlock();  
            try {
                connection_processing(elem, _kq); // solution task
            }
            catch (std::exception& err){
                std::cerr << err.what() << std::endl;
            }
        }
    }
}

ThreadPoll::ThreadPoll(int cnt_threads, const int& kq): _kq(kq){
    stopped = false; // should always be false until the destructor is called. It indicates that the thread pool is still alive and able to process incoming tasks.
    paused = true; // Waiting mode
    for (int i = 0; i < cnt_threads; ++i) {
        Thread *th = new Thread;
        th->_thread = std::thread(&ThreadPoll::worker_func, this, th);
        th->is_working = false; 
        workers.push_back(th);
    }
}

ThreadPoll::~ThreadPoll(){
    stopped = true;
    tasks_access.notify_all();
	for (auto& thread : workers) {
		thread->_thread.join();
		delete thread;
	}
}

void ThreadPoll::start(){
    if (paused){
        paused = false;
        tasks_access.notify_all(); // Дает всем потокам доступ к очереди невыполненных задач
    }
}

void ThreadPoll::stop(){
    paused = true;
}

void ThreadPoll::add_task(int client_socket){
    std::lock_guard<std::mutex> lock(task_queue_mut);
    tasks_queue.push(client_socket);
    tasks_access.notify_one(); // Разбудить один поток
}