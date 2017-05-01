#include <stdio.h>

int fibonacci(int x) {
	if (x == 1 || x == 2) return 1;
	return fibonacci(x - 1) + fibonacci(x - 2);
}
int main() {
	int i;
	for (i = 1; i < 10; i++) 
		printf("Fib(%d)=%d\n", i, fibonacci(i));
}
