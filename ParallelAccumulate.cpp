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
		std::unique_lock lock(mutex_);
		queue_not_full_.wait(lock, [this] {return !this->is_full(); });
		queue_.push_back(value);
		queue_not_empty_.notify_one();
	}

	int pop()
	{
		std::unique_lock lock(mutex_);
		queue_not_empty_.wait(lock, [this] {return !this->is_empty(); });
		int value = queue_.front();
		queue_.pop_front();
		queue_not_full_.notify_one();
		return value;
	}

private:
	bool is_full() const
	{
		return queue_.size() >= max_size;
	}

	bool is_empty() const
	{
		return queue_.empty();
	}

	const std::size_t max_size = 3;
	std::mutex mutex_;
	std::list<int> queue_;
	std::condition_variable queue_not_full_;
	std::condition_variable queue_not_empty_;
};

class Producer
{
public:
	Producer(Queue& queue) : queue_(queue) {}

	auto run() {
		return std::async([=, this] {for (int i = 0; i < 1000000001; i++) { this->queue_.push(i); }});
	}

private:
	Queue& queue_;
};

class Consumer {
public:
	Consumer(Queue& queue) : queue_(queue), result(0) {}

	auto run() {
		return std::async([this] {for (int i = 0; i < 10000; i++) {
			int number = this->queue_.pop();
			result += number; 
		}});
	}

private:
	Queue& queue_;
	long long result;
};


void main()
{
	Queue q;
	Producer p(q);
	Consumer c(q);

	auto future_producer = p.run();
	auto future_consumer = c.run();

	future_producer.wait();
	future_consumer.wait();
}

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

/*
int main()
{
	Accumulator acc;

	// Measure time of normal accumulate
	auto start = std::chrono::system_clock::now();
	unsigned long long result = 0;
	acc.accumulate(result, 1000000001);
	std::cout << result << "\n";
	auto end = std::chrono::system_clock::now();
	auto elapsed = (end - start).count();
	std::cout << "Elapsed time of normal accumulate: " << elapsed << "\n\n";

	// Mesure time of parallel accumulate
	start = std::chrono::system_clock::now();
	std::cout << acc.parallelAccumulate(1000000001) << "\n";
	end = std::chrono::system_clock::now();
	auto elapsed2 = (end - start).count();
	std::cout << "Elapsed time of parallel accumulate: " << elapsed2 << "\n\n";

	float ratio = static_cast<float>(elapsed) / elapsed2;
	std::cout << "Parallel version is " << ratio << "x faster" << "\n\n\n";

	return 0;
}*/