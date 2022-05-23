#pragma once

#ifndef __THREADSAFE_MAP_H__
#define __THREADSAFE_MAP_H__

#include <mutex>
#include <functional>
#include <unordered_map>
#include <list>
#include "boost/thread/shared_mutex.hpp"
#include "boost/thread/locks.hpp"
#include <algorithm>
#include <memory>
THREADSAFT_CONTAINER_BEGIN
const static unsigned int _gPrimes[] =
{
    53, 97, 193, 389, 769,1543,
    3079, 6151, 12289, 24593, 49157, 98317
};

template<class Key, class Value, class Hash = std::hash<Key>>
class threadsafe_map
{
private:
    class BucketType
    {
    private:
        using BucketValue = std::pair<Key, Value>;
        using BucketData = std::list<BucketValue>;
        using BucketInterator = std::list<BucketValue>::iterator;
        BucketData _data;
        mutable boost::shared_mutex _rwm;

    private:
        BucketInterator findKeyEntry(const Key& key) const
        {
            return std::find_if(_data.begin(), _data.end(), [](const BucketValue& elem) {return elem.first == key; });
        }

    public:
        Value getValue(const Key& key, const Key& defaultValue) const
        {
            boost::shared_lock<boost::shared_mutex> readLock(_rwm);
            const BucketInterator entryPosi = findKeyEntry(key);
            return entryPosi == _data.end() ? defaultValue : entryPosi->second;
        }

        void addPair(const Key& key, const Value& value)
        {
            std::unique_lock<boost::shared_mutex> writeLock(_rwm);
            const BucketInterator entryPosi = findKeyEntry(key);
            if (entryPosi == _data.end())
            {
                _data.emplace_back(BucketValue{ key, value });
            }
            else
            {
                entryPosi->second = value;
            }
        }

        void removePair(const Key& key)
        {
            std::unique_lock<boost::shared_mutex> writeLock(_rwm);
            const BucketInterator entryPosi = findKeyEntry(key);
            if (entryPosi != _data.end())
            {
                _data.erase(entryPosi);
            }
        }
    };

    std::vector<std::unique_ptr<BucketType>> _buckets;
    Hash _hashFunc;
    std::atomic<int> _realSize = 0;
    unsigned int findNextPrime(int size)
    {
        int len = sizeof(_gPrimes) / sizeof(_gPrimes[0]);
        int dstPrime = _gPrimes[0];
        for (int i = 0; i < len; ++i)
        {
            if (_gPrimes[i] <= size)
            {
                continue;
            }
            dstPrime = _gPrimes[i];
            break;
        }
        return dstPrime;
    }

    BucketType& getBucket(const Key& key) const
    {
        const size_t bucketIndex = _hashFunc(key) % _buckets.size();
        return *(_buckets[bucketIndex]);
    }

public:
    threadsafe_map(unsigned int pairsNum = 53, const Hash& hashFun = Hash()):_hashFunc(hashFun)
    {
        _buckets.resize(pairsNum);
        for (int i = 0; i < pairsNum; ++i)
        {
            _buckets[i].reset(new BucketType);
        }
    }

    threadsafe_map(const threadsafe_map& rhs) = delete;
    threadsafe_map& operator=(const threadsafe_map& rhs) = delete;
};
THREADSAFT_CONTAINER_END
#endif //!__THREADSAFE_MAP_H__
