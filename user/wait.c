#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main (int argc, char *argv[]) {
	if (argc != 2) {
		fprintf(2, "Usage: wait <time>\n");
		exit(1);
	}
	int begin_time = uptime();
	int wait_time = atoi(argv[1]);
	while (uptime() - begin_time < wait_time)
		;
	printf("waited for %d ticks\n", wait_time);
	exit(0);
}
