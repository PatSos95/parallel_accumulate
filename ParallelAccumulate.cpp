#include <iostream>
#include <thread>
#include <vector>
#include <future>
#include <chrono>
#include <cstdint>
#include <functional>
#include <condition_variable>
#include <mutex>
#include <list>

class Queue
{
public:
	void push(int value)
	{
		std::unique_lock lock(queueMutex);
		queueNotFull.wait(lock, [this] {return !this->isFull(); });
		queue.push_back(value);
		queueNotEmpty.notify_one();
	}

	int pop()
	{
		std::unique_lock lock(queueMutex);
		queueNotEmpty.wait(lock, [this] {return !this->isEmpty(); });
		int value = queue.front();
		queue.pop_front();
		queueNotFull.notify_one();
		return value;
	}

	unsigned int size()
	{
		return queue.size();
	}

private:
	bool isFull() const
	{
		return queue.size() >= maxSize;
	}

	bool isEmpty() const
	{
		return queue.empty();
	}

	const std::size_t maxSize = 3;
	std::mutex queueMutex;
	std::list<int> queue;
	std::condition_variable queueNotFull;
	std::condition_variable queueNotEmpty;
};

class Producer
{
public:
	Producer(Queue& queue) : queue(queue) {}

	auto generateNumbers(unsigned int lastNumber, unsigned int firstNumber = 0)
	{
		return std::async([=, this] {
			for (int i = firstNumber; i < lastNumber + 1; i++)
			{ 
				this->queue.push(i); 
			}});
	}

private:
	Queue& queue;
};


class Accumulator
{
public:
	Accumulator(Queue& queue) : queue(queue) {}

	auto accumulate(unsigned int lastNumber, unsigned int firstNumber = 0)
	{
		return std::async([=, this] {
			unsigned long long result = 0;
			for (int i = firstNumber; i <= lastNumber; i++)
			{
				int number = this->queue.pop();
				result += number;
			}
			return result;
		});
	}

private:
	std::mutex accumulateMutex;
	Queue& queue;
};

int main()
{
	unsigned int number = 100000;
	unsigned long long result = 0;

	Queue queue;
	Producer producer1(queue);
	Producer producer2(queue);
	Producer producer3(queue);
	Accumulator acc1(queue);
	Accumulator acc2(queue);

	auto futureProducer1 = producer1.generateNumbers(number / 3);
	auto futureProducer2 = producer2.generateNumbers((number / 3) * 2, (number / 3) + 1);
	auto futureProducer3 = producer3.generateNumbers(number, (number / 3) * 2 + 1);
	auto futureAcc1 = acc1.accumulate(number / 2);
	auto futureAcc2 = acc2.accumulate(number, (number / 2) + 1);

	futureProducer1.wait();
	futureProducer2.wait();
	futureProducer3.wait();
	std::cout << futureAcc1.get() + futureAcc2.get() << "\n\n";

	return 0;
}

