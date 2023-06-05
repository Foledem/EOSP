# EOSP Threadpool

## Description

**EOSP ThreadPool** is a cross-platform header-only templated thread pool writtent in pure c++17 using futre/promise. It is designed to be easy to use while being able to execute any type of function. You only need two functions to use the thread pool. `addTask()`to add a function to execute and `waitForTask()` to retrieve the return value and ensure the task is completed.

### No lazy thread
When threads are waiting for a result from other threads they compute new functions from the task pool until the result they need is available. They never wait without doing anything except if the pool is empty. The task pool is common to all threads. It is therefore not necessary to implement task stealing

### Priority queue
It is very simple to choose a priority level for the execution of a task

## Instalation

The installation is very simple. The ThreadPool is header-only so you just need to include the header to your project.

## Exemple

~~~~

#include <future>
#include <iostream>
#include "eosp/Threadpool.hpp"


using namespace std;

int f1(int x, int y)
{
	return x+y;
}

void f2()
{
    cout << "Hello World !" << endl;
}

int main()
{
    ThreadPool Tpool;				//create a default thread pool

	std::future<void> fut2 = Tpool.addTask(f2);		//add f2 in the task queue (priority = 0 by default)

	std::future<int> fut1 = Tpool.addTask(10, f1, 7, 31);	//add f1 in the task queue with a 10 priority value
    int result = Tpool.waitForTask(fut1);	//wait the result of f1 and store the return value

    Tpool.waitForTask(fut2);			//wait the end of the execution of f2 (void function)

    cout << result << endl;
	
	return 0;		//The destructor of the threadpool wait the end of each thread before the destruction
}
~~~~

* * *

When threads begin working on new tasks before completing the first task, it can result in recursion. `ThreadPool` can be used with parameters to chose the number of thread and the maximum recursion depth.

`ThreadPool(nbrThread, depth)` creates a thread pool with `nbrThread` thread (default value is the number of CPU threads) And `depth` the maximum recursion depth which defines the number of times an unfinished task can start a new one to wait (default value is 5).

`addTask(fct, param)` adds a function `fct` with `param` as parameter to the pool to be compute. There can be no parameter, one or more. `addTask()` return an `std::future<T>` where T is the return type of `fct`function.

`WaitForTask(fut)` waits the task is done if it isn't yet and return its return value if there is one. It has the same return type as `fct` or no return type if `fct` is a void function.

## Future improvements

* Task Manager

Currently all threads share the same task pool to execute. This model is simple but can be slowed down when a large number of threads try to take on a new task because they must share a mutex to access the task pool. Introducing a task manager distributing tasks to threads instead of letting threads fight for the mutex should improve performance under these conditions.

* Task cancellation

For any reason, it may be interesting to cancel a task, such as when the data is out of date
