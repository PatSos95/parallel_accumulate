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

	const std::size_t maxSize = 1;
	std::mutex queueMutex;
	std::list<int> queue;
	std::condition_variable queueNotFull;
	std::condition_variable queueNotEmpty;
};

class Producer
{
public:
	Producer(Queue& queue) : queue(queue) {}

	auto run(unsigned int numberOfGeneratedNumbers) 
	{
		return std::async([=, this] {
			for (int i = 0; i < numberOfGeneratedNumbers + 1; i++) 
			{ 
				this->queue.push(i); 
			}});
	}

private:
	Queue& queue;
};

class Consumer {
public:
	Consumer(Queue& queue) : queue(queue), result(0) {}

	auto run(unsigned int numberOfNumbersToPop)
	{
		return std::async([=, this] {
			for (int i = 0; i < numberOfNumbersToPop + 1; i++)
			{
				int number = this->queue.pop();
				result += number;
			}
			return result; 
		});
	}

private:
	Queue& queue;
	long long result;
};


class Accumulator
{
public:
	void accumulate(unsigned long long& total_result, unsigned int lastNumber, unsigned int firstNumber = 1)
	{
		unsigned long long result = 0;
		while (lastNumber >= firstNumber)
		{
			result += lastNumber;
			lastNumber -= 1;
		}

		std::lock_guard<std::mutex> lock(accumulateMutex);
		total_result += result;
	}

	unsigned long long parallelAccumulate(unsigned int lastNumber)
	{
		// Prepare variables
		const unsigned int numOfThreads = std::thread::hardware_concurrency() - 1; // (1) - one thread for main thread
		std::vector<std::future<void>> futuresVector;
		const unsigned int numbersPerGroup = lastNumber / numOfThreads;
		unsigned int firstNumberOfGroup;
		std::future<void> future;
		unsigned long long result = 0;

		// Split set on threads
		for (int i = 0; i < numOfThreads - 1; i++)
		{
			firstNumberOfGroup = lastNumber - numbersPerGroup + 1;
			future = std::async(std::launch::async, [=, &result] {this->accumulate(std::ref(result), lastNumber, firstNumberOfGroup); });
			futuresVector.push_back(std::move(future));
			lastNumber = firstNumberOfGroup - 1;
		}
		future = std::async(std::launch::async, [=, &result] {this->accumulate(std::ref(result), lastNumber, 1); });
		futuresVector.push_back(std::move(future));

		// Accumulate result in new thread
		for (auto& f : futuresVector)
		{
			f.wait();
		}

		return result;
	}

private:
	std::mutex accumulateMutex;
};

int main()
{
	unsigned int number = 100;

	Queue q;
	Producer p(q);
	Consumer c(q);

	auto future_producer = p.run(number);
	auto future_consumer = c.run(number);

	future_producer.wait();
	std::cout << future_consumer.get() << "\n\n";


	Accumulator acc;

	// Measure time of normal accumulate
	auto start = std::chrono::system_clock::now();
	unsigned long long result = 0;
	acc.accumulate(result, number);
	std::cout << result << "\n";
	auto end = std::chrono::system_clock::now();
	auto elapsed = (end - start).count();
	std::cout << "Elapsed time of normal accumulate: " << elapsed << "\n\n";

	// Mesure time of parallel accumulate
	start = std::chrono::system_clock::now();
	std::cout << acc.parallelAccumulate(number) << "\n";
	end = std::chrono::system_clock::now();
	auto elapsed2 = (end - start).count();
	std::cout << "Elapsed time of parallel accumulate: " << elapsed2 << "\n\n";

	float ratio = static_cast<float>(elapsed) / elapsed2;
	std::cout << "Parallel version is " << ratio << "x faster" << "\n\n\n";

	return 0;
}

