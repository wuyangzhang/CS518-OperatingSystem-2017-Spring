#include <stdio.h>
#include <string.h>

struct x {
	float val;
	char buf[sizeof(float)];
};

union y {
	float val;
	char buf[sizeof(float)];
};


int main() {
	struct x str1;
	memset(&str1, 0, sizeof(struct x));
	str1.val = 35;
	printf("%02x %02x %02x %02x\n", str1.buf[0], str1.buf[1], *(str1.buf + 2), str1.buf[3]);

	union y str2;
	memset(&str2, 0, sizeof(union y));
	str2.val = 35;
	printf("%02x %02x %02x %02x\n", str2.buf[0], str2.buf[1], *(str2.buf + 2), str2.buf[3]);

	float f = 35;
	char *buf2 = (char *)&f;
	printf("%02x %02x %02x %02x\n", buf2[0], buf2[1], buf2[2], buf2[3]);

}
