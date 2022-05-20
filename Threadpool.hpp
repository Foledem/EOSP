#ifndef THREADPOOL_HPP
#define THREADPOOL_HPP

#include <atomic>
#include <functional>
#include <future>
#include <map>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>
#include <vector>

class ThreadPool
{
public:
    ThreadPool(unsigned int thread = std::thread::hardware_concurrency(), unsigned int maxRecursionDepth=5):
        maxRecursion(maxRecursionDepth),
        threadNumber(thread)
    {
        if(threadNumber==0)
            threadNumber=std::thread::hardware_concurrency();

        for(unsigned int i=0;i<threadNumber;i++)
        {
            RecursionMap.emplace(std::hash<std::thread::id>()(workers.emplace_back(std::thread([this]{workerLoop();})).get_id()), 0);//Create thread, fill map with hash of thread id and set recursion to 0
        }
        RecursionMap.emplace(std::hash<std::thread::id>()(std::this_thread::get_id()),0);//add hash of the main thread in the map
    }
    ~ThreadPool()
    {
        stop = true;
        cv.notify_all();

        for(unsigned int i=0;i<threadNumber;i++)
        {
            workers.at(i).join();
        }

    }

    template <typename F, typename...Args>
    auto addTask(F&& f, Args&&... args) -> std::future<decltype (f(args...))>
    {
        using ReturnType = decltype(f(args...));

        if(stop)
            return{};

        if(RecursionMap.find(std::hash<std::thread::id>()(std::this_thread::get_id()))->second < maxRecursion)//if recursion isn't too deep
        {

            std::function<ReturnType()> func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
            auto pTask = std::make_shared<std::packaged_task<ReturnType()>>(func);
            std::function<void()> wrapperFunc = [pTask](){(*pTask)();};

            auto result = pTask->get_future();
            {
                std::lock_guard lk(taskFrame.mtx);
                taskFrame.taskQueue.emplace(wrapperFunc);//add task in queue
                cv.notify_one();
            }
            return result;
        }
        else//if recursion is too deep do not add in the queue. Direct execute
        {
            std::promise<ReturnType> result;
            if constexpr(std::is_same_v<ReturnType, void>)
            {
                f(args...);
                result.set_value();
            }
            else
            {
                result.set_value(f(args...));
            }
            return result.get_future();
        }
    }

    template <typename T>
    auto waitForTask(std::future<T>& fut)
    {
        while(!stop)
        {
            std::unique_lock lock(taskFrame.mtx);
            if(fut.wait_for(std::chrono::seconds(0))==std::future_status::ready)//if the task is done then return the result
                return fut.get();

            tryToWork(lock);//else take an other task
        }
        return static_cast<T>(NULL);//void or other
    }

private:

    void tryToWork(std::unique_lock<std::mutex>& lock)
    {

        if(stop)
            return;
        if(taskFrame.taskQueue.size() == 0)//if there is no task in queue
        {
            cv.wait(lock);//wait other threads
            return;
        }

        std::function<void()> task = std::move((taskFrame.taskQueue.front()));//else take one in the queue
        taskFrame.taskQueue.pop();
        lock.unlock();
        RecursionMap.find(std::hash<std::thread::id>()(std::this_thread::get_id()))->second ++;//increment recursion
        task();
        RecursionMap.find(std::hash<std::thread::id>()(std::this_thread::get_id()))->second --;
        std::lock_guard notifyLock(taskFrame.mtx);
        cv.notify_all();

    }

    void workerLoop()
    {
        while(!stop)
        {
            std::unique_lock lock(taskFrame.mtx);
            tryToWork(lock);//take a task in the queue if there are otherwise wait
        };
    }

private:

    std::atomic<bool> stop {false};
    unsigned int maxRecursion;
    unsigned int threadNumber;
    std::condition_variable cv;

    struct {
    std::mutex mtx;
    std::queue<std::function<void()>> taskQueue;//FIFO logic
    } taskFrame;

    std::vector<std::thread> workers;
    std::map<size_t, unsigned int> RecursionMap;

};

#endif // THREADPOOL_HPP
