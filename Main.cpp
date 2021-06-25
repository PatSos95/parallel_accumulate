

/*
int main()
{
	Queue q;
	Producer p(q);
	Consumer c(q);

	auto future_producer = p.run();
	auto future_consumer = c.run();

	future_producer.wait();
	future_consumer.wait();

	
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