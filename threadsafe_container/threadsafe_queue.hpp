#pragma once

#ifndef __THREADSAFE_QUEUE_H__
#define __THREADSAFE_QUEUE_H__

#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>

THREADSAFT_CONTAINER_BEGIN

template <class T>
class threadsafe_queue
{
public:
    threadsafe_queue() 
    {
        _head = std::make_unique<node>(node{});
        _tail = _head.get();
    };
    threadsafe_queue(const threadsafe_queue& rhs) = delete;
    threadsafe_queue& operator=(const threadsafe_queue&) = delete;
    ~threadsafe_queue() {}
    void push(const T newValue)
    {
        std::shared_ptr<T> newData = std::make_shared<T>(std::move(newValue));
        std::unique_ptr<node> p = std::make_unique<node>(node{});
        {
            std::lock_guard<std::mutex> tailLock(_tailMutex);
            _tail->_data = newData;
            node* const newTail = p.get();
            _tail->_next = std::move(p);
            _tail = newTail;
        }
        _dataCond.notify_one();
    }

    std::shared_ptr<T> wait_and_pop()
    {
        std::unique_ptr<node> const oldHead = wait_pop_head();
        return oldHead->_data;
    }

    void wait_and_pop(T& value)
    {
        wait_pop_head(value);
    }

    std::shared_ptr<T> try_pop()
    {
        std::unique_ptr<node> oldHead = try_pop_head();
        return oldHead ? oldHead->_data : nullptr;
    }

    bool try_pop(T& value)
    {
        std::unique_ptr<node> const oldHead = try_pop_head(value);
        return oldHead;
    }

    bool empty()
    {
        std::lock_guard<std::mutex> headLock(_headMutex);
        return (_head.get() == get_tail());
    }
private:
    struct node
    {
        std::shared_ptr<T> _data;
        std::unique_ptr<node> _next;
    };

    std::unique_ptr<node> _head;
    std::mutex _headMutex;
    node* _tail;
    std::mutex _tailMutex;
    std::condition_variable _dataCond;

private:
    node* get_tail()
    {
        std::lock_guard<std::mutex> tailLock(_tailMutex);
        return _tail;
    }

    std::unique_ptr<node> pop_head()
    {
        std::unique_ptr<node> oldHead = std::move(_head);
        _head = std::move(oldHead->_next);
        return oldHead;
    }

    std::unique_ptr<node> wait_pop_head()
    {
        std::unique_lock<std::mutex> headLock(_headMutex);
        _dataCond.wait(headLock, [&]() {return _head.get() != get_tail(); });
        return pop_head();
    }

    std::unique_ptr<node> wait_pop_head(T& value)
    {
        std::unique_lock<std::mutex> headLock(_headMutex);
        _dataCond.wait(headLock, [&]() {return _head.get() != get_tail(); });
        value = std::move(*(_head->_data));
        return pop_head();
    }

    std::unique_ptr<node> try_pop_head()
    {
        std::lock_guard<std::mutex> headLock(_headMutex);
        if (_head.get() == get_tail())
        {
            return std::unique_ptr<node>();
        }
        return pop_head();
    }

    std::unique_ptr<node> try_pop_head(T& value)
    {
        std::lock_guard<std::mutex> headLock(_headMutex);
        if (_head.get() == get_tail())
        {
            return std::unique_ptr<node>();
        }
        value = std::move(*_head->_data);
        return pop_head();
    }
};

THREADSAFT_CONTAINER_END
#endif //__THREADSAFE_QUEUE_H__