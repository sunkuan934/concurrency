#pragma once
#ifndef __SIMPLE_THREAD_POOL_H__
#define __SIMPLE_THREAD_POOL_H__

#include <iostream>
#include "my_algorithm.hpp"
#include <random>
#include <ctime>
#include "thread_saft_stack.h"
#include "threadsafe_queue.h"
#include <chrono>
#include "future_learning.h"
#include "template_dispatcher.h"
#include <future>
#include <memory>


class SimpleThreadPool
{
public:
	SimpleThreadPool(size_t threadSize = 1);
	~SimpleThreadPool();
	template<class F, class... Args>
	auto enqueue(F&& f, Args&&... args)->std::future<decltype(f(args...))>;

	void join_all();
private:
	std::vector<std::thread> _workThreads;
	threadsafe_queue<std::function<void()>> _tasks;
	std::mutex _threadPoolLock;
	std::condition_variable _threadPoolCv;
	std::atomic<bool> _end = false;
};

SimpleThreadPool::SimpleThreadPool(size_t threadSize)
{
	for (size_t i = 0; i < threadSize; ++i)
	{
		_workThreads.emplace_back(
			[this] () {
				while (true)
				{
					std::shared_ptr<std::function<void()>> task;
					{
						std::unique_lock<std::mutex> lock(this->_threadPoolLock);
						this->_threadPoolCv.wait(lock, [this]() { return !this->_tasks.empty() || this->_end; });
					}
					if (this->_end && this->_tasks.empty())
						return;
					if (task = this->_tasks.wait_and_pop())
					{
						(*task)();
					}
				}
			}
		);
	}
}

template<class F, class... Args>
auto SimpleThreadPool::enqueue(F&& f, Args&&... args)->std::future<decltype(f(args...))>
{
	using returnType = decltype(f(args...));

	auto currentTask = std::make_shared<std::packaged_task<returnType()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
	std::future<returnType> res = currentTask->get_future();
	{
		std::function<void()> taskFunc = [currentTask]() { (*currentTask)(); };
		_tasks.push(std::move(taskFunc));
	}
	_threadPoolCv.notify_one();
	return res;
}

SimpleThreadPool::~SimpleThreadPool()
{
	_end = true;
	_threadPoolCv.notify_all();
	for (auto& eachWorkThread : _workThreads)
	{
		if (eachWorkThread.joinable())
		{
			eachWorkThread.join();
		}
	}
}

void SimpleThreadPool::join_all()
{
	_end = true;
	_threadPoolCv.notify_all();
	for (auto& eachWorkThread : _workThreads)
	{
		if (eachWorkThread.joinable())
		{
			eachWorkThread.join();
		}
	}
}

#endif // !__SIMPLE_THREAD_POOL_H__
