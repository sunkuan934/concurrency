#include <iostream>
#include <thread>
#include "my_algorithm.hpp"
#include "thread_saft_stack.h"
#include "threadsafe_queue.h"
#include "simple_thread_pool.hpp"
#include "threadsafe_map.hpp"
#include "logger.h"

using threadsaft_container::threadsafe_queue;
using parallel_algorithm::parallel_sort;

const int THREAD_POOL_SIZE = 4;
class SimpleMessageQueue
{
public:
    SimpleMessageQueue(int index) :_index(index){}
    SimpleMessageQueue(const SimpleMessageQueue& rhs) :_index(rhs._index) {}
    SimpleMessageQueue& operator=(const SimpleMessageQueue& rhs) { _index = rhs._index;  return *this; }
public:
    void receive_message(int index)
    {
        for (int i = 0; i < 6000000; ++i) //简单模拟生产者，将数据放进消息队列中
        {
            std::default_random_engine random_engine(std::chrono::steady_clock::now().time_since_epoch().count());
            std::uniform_int_distribution<unsigned int> value(0, 10000);
            int num = value(random_engine);
            _tq.push(num);
            ++_gCount;
        }
        _return = true;
    }

    void process_message(int index)
    {
        while (true)
        {
            while (!_tq.empty()) //简单模拟消费者，从消息队列取出消息并处理
            {
                auto res = _tq.wait_and_pop();
                if (res != nullptr)
                {
                    ++_gCount2;
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
    Log::Logger& log = Log::Logger::getInstance();
    log.setLogLevel(Log::LogLevel::DEBUG); //设置日志级别

    std::vector<SimpleMessageQueue> tp;
    for (int i = 0; i < 4; ++i)
    {
        tp.push_back(std::move(SimpleMessageQueue{i}));
    }
    do {
        SimpleThreadPool pushThreadPool(THREAD_POOL_SIZE);
        SimpleThreadPool popThreadPool(2 * THREAD_POOL_SIZE);
        auto start = std::chrono::steady_clock::now();
        for (int i = 0; i < THREAD_POOL_SIZE; ++i) //生产者线程池
        {
            auto task = std::bind(&SimpleMessageQueue::receive_message, std::ref(tp[i]), i);
            pushThreadPool.enqueue(task);
        }

        for (int i = 0; i < 2 * THREAD_POOL_SIZE; ++i) //消费者线程池
        {
            auto task = [&tp, i]() {tp[i % THREAD_POOL_SIZE].process_message(i); };
            popThreadPool.enqueue(task);
        }

        pushThreadPool.join_all(); //主动join()，也可以注释掉，通过SimpleThreadPool的析构去join()
        popThreadPool.join_all();
    } while(false);

    log(Log::LogLevel::INFO) << SimpleMessageQueue::_gCount.load();
    return 0;
}
