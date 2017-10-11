#include <iostream>

#include <scheduler.hpp>

using namespace staccato;

int fib(int n)
{
    int a = 1, b = 1;
    for (int i = 3; i <= n; i++) {
        int c = a + b;
        a = b;
        b = c;
    }           
    return b;
}


int main(int argc, char *argv[])
{
	unsigned n = 20;
	long answer;

	if (argc == 2) {
		n = atoi(argv[1]);
	}

	scheduler::initialize();

	scheduler::spawn([&]() {
		answer = fib(n);
	});

	scheduler::wait();

	scheduler::terminate();

	std::cout << "fib(" << n << ") = " << answer << "\n";

	return 0;
}
