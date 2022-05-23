#pragma once
#ifndef __THREADSAFE_STACK_H__
#define __THREADSAFE_STACK_H__
#include <memory>
#include <mutex>
#include <stack>
#include "base_def.h"

THREADSAFT_CONTAINER_BEGIN

template <class T>
class threadsaft_stack
{
public:
    using SmartPtr4T = std::shared_ptr<T>;

public:
    threadsaft_stack() {}
    threadsaft_stack(const threadsaft_stack& rhs)
    {
        std::lock_guard<std::mutex> l(rhs._m);
        _data = rhs._data;
    }

    threadsaft_stack& operator = (const threadsaft_stack&) = delete;

    void push(T newValue)
    {
        SmartPtr4T data = std::make_shared<T>(std::move(newValue));
        std::lock_guard<std::mutex> l(_m);
        _data.push(data);
    }

    bool try_pop(T &value)
    {
        std::lock_guard<std::mutex> l(_m);
        if (!_data.empty())
        {
            value = std::move(*_data.top());
            _data.pop();
            return true;
        }
        return false;
    }

    SmartPtr4T try_pop()
    {
        std::lock_guard<std::mutex> l(_m);
        if (!_data.empty())
        {
            SmartPtr4T res = _data.top();
            _data.pop();
            return res;
        }
        return nullptr;
    }

    void wait_and_pop(T &value)
    {
        std::unique_lock<std::mutex> ul(_m);
        _cv.wait(ul, [this]() {return !_data.empty(); });
        value = std::move(*_data.top());
        _data.pop();
    }

    SmartPtr4T wait_and_pop()
    {
        std::unique_lock<std::mutex> ul(_m);
        _cv.wait(ul, [this]() {return !_data.empty(); });
        SmartPtr4T res = _data.top();
        _data.pop();
        return res;
    }

    bool empty() const
    {
        std::lock_guard<std::mutex> l(_m);
        return _data.empty();
    }

    friend void swap(threadsaft_stack<T>& lhs, threadsaft_stack<T>& rhs)
    {
        if (&lhs == &rhs)
        {
            return;
        }

        std::lock(lhs._m, rhs._m);
        std::lock_guard<std::mutex> lock_lhs(lhs._m, std::adopt_lock);
        std::lock_guard<std::mutex> lock_rhs(rhs._m, std::adopt_lock);
        swap(lhs._data, rhs._data);
    }
private:
    std::stack<SmartPtr4T> _data;
    mutable std::mutex _m;
    std::condition_variable _cv;
};

THREADSAFT_CONTAINER_END

#endif // !__THREADSAFE_STACK_H__
