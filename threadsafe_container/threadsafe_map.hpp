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
    53,   97,   193,   389,   769,   1543,
    3079, 6151, 12289, 24593, 49157, 98317
};

template<class Key, class Value, class Hash = std::hash<Key>>
class threadsafe_map
{
private:
    class BucketType
    {
    public:
        using BucketValue = std::pair<Key, Value>;
        using BucketData = std::list<BucketValue>;
        using BucketInterator = BucketData::iterator;
        using BucketConstInterator = BucketData::const_iterator;
        BucketData _data;
        mutable boost::shared_mutex _rwm;

    public:

        BucketInterator findKeyEntry(Key const& key)
        {
            return std::find_if(_data.begin(), _data.end(), [&](BucketValue const& elem) {return elem.first == key; });
        }

        BucketConstInterator findKeyEntry(Key const& key) const
        {
            return std::find_if(_data.begin(), _data.end(), [&](BucketValue const& elem) {return elem.first == key; });
        }

    public:
        BucketData& getBucketData()
        {
            return _data;
        }

        Value getValue(Key const& key, const Value& defaultValue) const
        {
            boost::shared_lock<boost::shared_mutex> readLock(_rwm);
            BucketConstInterator entryPosi = findKeyEntry(key);
            return entryPosi == _data.end() ? defaultValue : entryPosi->second;
        }

        void addPair(const Key& key, const Value& value)
        {
            std::unique_lock<boost::shared_mutex> writeLock(_rwm);
            BucketInterator entryPosi = findKeyEntry(key);
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
            BucketInterator entryPosi = findKeyEntry(key);
            if (entryPosi != _data.end())
            {
                _data.erase(entryPosi);
            }
        }
    };

    std::vector<std::unique_ptr<BucketType>> _buckets;
    Hash _hashFunc;
    std::atomic<int> _realSize = 0;
    std::mutex _bucketsLock;

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

    Value getValue(const Key& key, const Value& defaultValue = Value()) const
    {
        return getBucket(key).getValue(key, defaultValue);
    }

    void addPair(const Key& key, const Value& value)
    {
        if (_realSize == _buckets.size())
        {
            std::lock_guard<std::mutex> lock(_bucketsLock);
            std::unordered_map<Key, Value> oriMap;
            for (int i = 0; i < _buckets.size(); ++i)
            {
                for (const auto& elem : _buckets[i]->getBucketData())
                {
                    oriMap.insert(elem);
                }
            }
            oriMap[key] = value;
            int newBucketsSize = findNextPrime(_realSize.load());
            _buckets.clear();
            _buckets.resize(newBucketsSize);
            for (int i = 0; i < newBucketsSize; ++i)
            {
                _buckets[i].reset(new BucketType);
            }
            for (const auto& elem : oriMap)
            {
                const size_t bucketIndex = _hashFunc(elem.first) % _buckets.size();
                (*_buckets[bucketIndex])._data.push_back(BucketType::BucketValue(elem.first, elem.second));
            }
        }
        else
        {
            getBucket(key).addPair(key, value);
        }
        ++_realSize;
    }

    void removePair(const Key& key)
    {
        getBucket(key).removePair(key);
    }
};
THREADSAFT_CONTAINER_END
#endif //!__THREADSAFE_MAP_H__
