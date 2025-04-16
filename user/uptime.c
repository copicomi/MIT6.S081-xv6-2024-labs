#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main (int argc, char *argv[]) {
	if (argc != 1) {
		fprintf(2, "Usage: uptime\n");
		exit(1);
	}
	printf("xv6 has run %d ticks\n", uptime());

	exit(0);
}
