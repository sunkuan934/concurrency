#include <iostream>
#include "my_algorithm.hpp"
#include <random>
#include <ctime>
#include "thread_saft_stack.h"
#include "threadsafe_queue.h"
#include <chrono>
#include "future_learning.h"
#include "template_dispatcher.h"
#include "boost/thread/thread_pool.hpp"
#include "boost/asio/thread_pool.hpp"
#include "simple_thread_pool.hpp"
#include "threadsafe_map.hpp"

//const int THREAD_POOL_SIZE = std::thread::hardware_concurrency();
const int THREAD_POOL_SIZE = 4;

void thread1(std::promise<bool>& res)
{
	srand((size_t)time(nullptr));
	int value = rand();
	res.set_value(value % 2 == 0);
}

void thread2(std::future<bool>& fut)
{
	bool res = fut.get();
	int value = res ? 5 : 10;
	std::cout << value << std::endl;
}

struct Sort
{
	Sort() {};
	template<class T>
	bool operator()(const std::vector<T>& nums, int left, int right)
	{
		if (left >= right)
		{
			return true;
		}
		for (int i = left; i < right; ++i)
		{
			if (nums[i] > nums[i + 1]) return false;
		}
		return true;
	}
};
class SimpleMessageQueue
{
public:
	SimpleMessageQueue(int index) :_index(index){}
	SimpleMessageQueue(const SimpleMessageQueue& rhs) :_index(rhs._index) {}
	SimpleMessageQueue& operator=(const SimpleMessageQueue& rhs) { _index = rhs._index;  return *this; }
public:
	void receive_message(int index)
	{
		for (int i = 0; i < 6; ++i)
		{
			std::default_random_engine random_engine(std::chrono::steady_clock::now().time_since_epoch().count());
			std::uniform_int_distribution<unsigned int> value(0, 10000);
			int num = value(random_engine);
			_tq.push(num);
			printf("Thread id:%d, index:%d, this num is : %d\n",std::this_thread::get_id(), index,  num);
			++_gCount;
		}
		_return = true;
	}

	void process_message(int index)
	{
		while (true)
		{
			while (!_tq.empty())
			{
				auto res = _tq.wait_and_pop();
				if (res != nullptr)
				{
					++_gCount2;
					printf("Thread Id : %d, Index : %d, Now Message is : %d\n", std::this_thread::get_id(), index, *res);
				}
			}
			if (_return)
			{
				return;
			}
		}
	}

	~SimpleMessageQueue()
	{
		while (!_tq.empty())
		{
			_tq.try_pop();
		}
		_return = true;
	}
public:
	threadsafe_queue<int> _tq;
	int _index = -1;
	std::atomic<bool> _return = false;
	static std::atomic<int> _gCount;
	static std::atomic<int> _gCount2;
};

std::atomic<int> SimpleMessageQueue::_gCount = 0;
std::atomic<int> SimpleMessageQueue::_gCount2 = 0;
int main()
{
	std::vector<SimpleMessageQueue> tp;
	for (int i = 0; i < 4; ++i)
	{
		tp.push_back(std::move(SimpleMessageQueue{i}));
	}
	SimpleThreadPool pushThreadPool(THREAD_POOL_SIZE);
	SimpleThreadPool popThreadPool(2 * THREAD_POOL_SIZE);
	auto start = std::chrono::steady_clock::now();
	for (int i = 0; i < THREAD_POOL_SIZE; ++i)
	{
		auto task = std::bind(&SimpleMessageQueue::receive_message, std::ref(tp[i]), i);
		pushThreadPool.enqueue(task);
	}

	for (int i = 0; i < 2 * THREAD_POOL_SIZE; ++i)
	{
		auto task = [&tp, i]() {tp[i % THREAD_POOL_SIZE].process_message(i); };
		popThreadPool.enqueue(task);
	}

	pushThreadPool.join_all();
	popThreadPool.join_all();
	auto end = std::chrono::steady_clock::now();
	std::cout << "Cost Time : " << std::chrono::duration_cast<std::chrono::seconds>(end - start) << std::endl;
	std::cout << "This Time count : " << SimpleMessageQueue::_gCount << std::endl;
	std::cout << "This Time count : " << SimpleMessageQueue::_gCount2 << std::endl;

	std::default_random_engine random_engine(std::chrono::steady_clock::now().time_since_epoch().count());
	std::uniform_int_distribution<unsigned int> uid(0, 10000);
	int size = uid(random_engine) + 100000;
	if (size > 0)
	{
		std::vector<int> test(size);
		for (int i = 0; i < size; ++i)
		{
			test[i] = uid(random_engine);
		}
		parallel_sort(test, 8);
		Sort st;
		std::cout << "res : " << st(test, 0, size - 1) << std::endl;
	}
	return 0;
}