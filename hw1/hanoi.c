#include <stdio.h>
#include <stdlib.h>

int x[14];// = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14};

void hanoi(int from, int to, int level) {
	if (level == 1) {
		printf("%d %d->%d\n", level, from, to);
	} else {
		hanoi(from, 6 - from - to, level - 1);
		printf("%d %d->%d\n", level, from, to);
		hanoi(6 - from - to, to, level - 1);		
	}
}

int main() {
	hanoi(1, 3, 4);
	int *x = malloc(sizeof(int));
}
