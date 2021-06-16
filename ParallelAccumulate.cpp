#include <iostream>
#include <thread>
#include <vector>
#include <future>
#include <chrono>
#include <cstdint>
#include <functional>

std::mutex accumulateMutex;
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
	const unsigned int numOfThreads = std::thread::hardware_concurrency() - 1; //(1) - one thread for main thread
	std::vector<std::future<void>> futuresVector;
	const unsigned int numbersPerGroup = lastNumber / numOfThreads;
	unsigned int firstNumberOfGroup;
	std::future<void> future;
	unsigned long long result = 0;

	// Split set on threads
	for (int i = 0; i < numOfThreads - 1; i++)
	{
		firstNumberOfGroup = lastNumber - numbersPerGroup + 1;
		future = std::async(std::launch::async, accumulate, std::ref(result), lastNumber, firstNumberOfGroup);
		futuresVector.push_back(std::move(future));
		lastNumber = firstNumberOfGroup - 1;
	}
	future = std::async(std::launch::async, accumulate, std::ref(result), lastNumber, 1);
	futuresVector.push_back(std::move(future));

	// Accumulate result in new thread
	for (auto& f : futuresVector)
	{
		f.wait();
	}

	return result;
}

int main()
{
	auto start = std::chrono::system_clock::now();
	unsigned long long result = 0;
	accumulate(result, 1000000001);
	std::cout << result << "\n";
	auto end = std::chrono::system_clock::now();
	auto elapsed = (end - start).count();
	std::cout << "Elapsed time of normal accumulate: " << elapsed << "\n\n";

	start = std::chrono::system_clock::now();
	std::cout << parallelAccumulate(1000000001) << "\n";
	end = std::chrono::system_clock::now();
	auto elapsed2 = (end - start).count();
	std::cout << "Elapsed time of parallel accumulate: " << elapsed2 << "\n\n";

	float ratio = static_cast<float>(elapsed) / elapsed2;
	std::cout << "Parallel version is " << ratio << "x faster" << "\n\n\n";

	return 0;
}