#pragma once

#ifndef __MY_ALGORITHM_H__
#define __MY_ALGORITHM_H__

#include <vector>
#include <numeric>
#include <thread>
#include <functional>
#include <algorithm>

using ULL = unsigned long long;
const ULL MIN_PER_THREAD = 25;
const ULL HARDWARE_THREADS = std::thread::hardware_concurrency();

template<class Iterator, class T>
struct AccumulateBlock {
	void operator()(Iterator first, Iterator last, T& result)
	{
		result = std::accumulate(first, last, result);
	}
};

template<class Iterator, class T>
T parallel_accumulate(Iterator first, Iterator last, T init)
{
	const ULL length = std::distance(first, last);
	if (length == 0)
	{
		return init;
	}
	const ULL maxThreads = (length + MIN_PER_THREAD - 1) / (MIN_PER_THREAD);
	const ULL threadNums = std::min(HARDWARE_THREADS != 0 ? HARDWARE_THREADS : 2, maxThreads);
	const ULL blockSize = length / threadNums;
	std::vector<T> results(threadNums);
	std::vector<std::thread> threads(threadNums - 1);

	Iterator blockStart = first;
	for (ULL i = 0; i < (threadNums - 1); ++i)
	{
		Iterator blockEnd = blockStart;
		std::advance(blockEnd, blockSize);
		threads[i] = std::thread(
			AccumulateBlock<Iterator, T>(),
			blockStart, blockEnd, std::ref(results[i])
		);
		blockStart = blockEnd;
	}

	AccumulateBlock<Iterator, T>()(blockStart, last, results[threadNums - 1]);
	std::for_each(threads.begin(), threads.end(), std::mem_fn(&std::thread::join));
	return std::accumulate(results.begin(), results.end(), init);
}

template<class T>
int partition(std::vector<T>& nums, int left, int right)
{
	srand(time(nullptr));
	int randPosi = rand() % (right - left + 1);
	std::swap(nums[left], nums[left + randPosi]);
	T pivot = nums[left];
	while (left < right)
	{
		while (left < right && nums[right] >= pivot) --right;
		if (left < right) nums[left] = nums[right];
		while (left < right && nums[left] <= pivot) ++left;
		if (left < right) nums[right] = nums[left];
	}
	nums[left] = pivot;
	return left;
}

template<class T>
void _quickSort(std::vector<T>& nums, int left, int right)
{
	if (left >= right) return;
	int mid = partition(nums, left, right);
	_quickSort(nums, left, mid - 1);
	_quickSort(nums, mid + 1, right);
}

template<class T>
void quick_sort(std::vector<T>& nums, int left, int right)
{
	_quickSort(nums, left, right);
}

template<class T>
void merge(std::vector<T>& nums, int left, int mid, int right)
{
	std::vector<T> temp(right - left + 1);
	int leftIdx = left;
	int rightIdx = mid + 1;
	int idx = 0;
	while (leftIdx <= mid && rightIdx <= right)
	{
		if (nums[leftIdx] < nums[rightIdx])
		{
			temp[idx++] = nums[leftIdx++];
		}
		else
		{
			temp[idx++] = nums[rightIdx++];
		}
	}
	while (leftIdx <= mid)
	{
		temp[idx++] = nums[leftIdx++];
	}
	while (rightIdx <= right)
	{

		temp[idx++] = nums[rightIdx++];
	}
	for (int i = left; i <= right; ++i)
	{
		nums[i] = temp[i - left];
	}
}

int log2num(int num)
{
	if (num <= 0) return -1;
	float f_num;
	unsigned long i_num, exp;
	f_num = (float)num;
	i_num = *(unsigned long*)&f_num;
	exp = (i_num >> 23) & 0xFF;
	return exp - 127;
}

template<class T>
void parallel_sort(std::vector<T>& nums, int dstThreadNum)
{
	int size = nums.size();
	if (size <= 200)
	{
		quick_sort(nums, 0, size - 1);
		return;
	}
	int cnt = 1;
	if (dstThreadNum >= 1)
	{
		cnt = log2num(dstThreadNum);
	}
	bool mergePeriod = false;
	int blockSize = 1;
	while (cnt)
	{
		int realThreadNum = pow(2, cnt);
		std::vector<std::thread> threads(realThreadNum);
		if (!mergePeriod)
		{
			blockSize = size / realThreadNum;
			for (int i = 0; i < realThreadNum - 1; ++i)
			{
				int left = i * blockSize;
				int right = (i + 1) * blockSize - 1;
				//threads[i] = std::thread(quick_sort<T>, std::ref(nums), start, (i + 1) * blockSize - 1 );
				threads[i] = std::thread(quick_sort<T>, std::ref(nums), left, right);
			}
			threads[realThreadNum - 1] = std::thread( quick_sort<T>, std::ref(nums), (realThreadNum - 1) * blockSize, size - 1 );
			std::for_each(threads.begin(), threads.end(), std::mem_fn(&std::thread::join));
			mergePeriod = true;
		}
		else
		{
			for (int i = 0; i < realThreadNum; ++i)
			{
				int gapIndex = 2 * i;
				int left = gapIndex * blockSize;
				int mid = (gapIndex + 1) * blockSize - 1;
				int right = (gapIndex + 2) * blockSize - 1;
				if (i == realThreadNum - 1)
				{
					right = size - 1;
				}
				threads[i] = std::thread( merge<T>, std::ref(nums), left, mid, right );
			}
			std::for_each(threads.begin(), threads.end(), std::mem_fn(&std::thread::join));
			blockSize *= 2;
		}
		--cnt;
	}
	merge(nums, 0, blockSize - 1, size - 1);
}
#endif //!__MY_ALGORITHM_H__
