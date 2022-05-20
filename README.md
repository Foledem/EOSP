# EOSP Threadpool

## Description

**EOSP ThreadPool** is a header-only templated thread pool writtent in c++17. It is designed to be easy to use while being able to execute any type of function. You only need two functions to use the thread pool. `addTask()`to add a function to execute and `waitForTask()` to get the return value and wait the task is done.
When threads are waiting for a result from other threads they compute new functions from the task pool until the result they need is available.

## Exemple

~~~~

#include <future>
#include <iostream>
#include "eosp/Threadpool.hpp"


using namespace std;

int f1(int x, int y)
{
    return x*(y%x);
}

void f2()
{
    cout << "Hello World !" << endl;
}

int main()
{
    ThreadPool Tpool;				//create a default thread pool

    std::future<void> fut2 = Tpool.addTask(f2);		//add f2 in the task queue	

    std::future<int> fut1 = Tpool.addTask(f1, 7, 31);	//add f1 in the task queue
    int result = Tpool.waitForTask(fut1);	//wait the result of f1 and store the return value

    Tpool.waitForTask(fut2);			//wait the end of the execution of f2 (void function)

    cout << result << endl;
}
~~~~

* * *

Since threads start to work on new tasks to wait before they return the first task, it can create some recursion. `ThreadPool` can be used with parameters to chose the number of thread and the maximum recursion depth.

`ThreadPool(nbrThread, depth)` creates a thread pool with `nbrThread` thread (default value is the number of CPU threads) And `depth` the maximum recursion depth which defines the number of times an unfinished task can start a new one to wait (default value is 5).

`addTask(fct, param)` adds a function `fct` with `param` as parameter to the pool to be compute. There can be no parameter, one or more. `addTask()` return an `std::future<T>` where T is the return type of `fct`function.

`WaitForTask(fut)` waits the task is done if it isn't yet and return its return value if there is one. It has the same return type as `fct` or no return type if `fct` is a void function.