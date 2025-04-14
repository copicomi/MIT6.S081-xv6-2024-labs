#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define DEBUG printf("this is a debug message\n");

int main (int argc, char *argv[]) {

	if (argc != 1) {
		fprintf(2, "Usage: primes\n");
		exit(1);
	}
	
	int fd0[2];

	if (pipe(fd0) < 0) {
		fprintf(2, "primes: pipe failed\n");
		exit(1);
	}

	int origin_flag = 1;
	if (fork() == 0) {
		origin_flag = 0;
	}

	// 原始进程，负责输入整数集合
	if (origin_flag == 1) {
		for (int i = 2; i <= 280; i ++) {
			if (write(fd0[1], &i, sizeof(int)) != sizeof(int)) {
				fprintf(2, "primes: pipe write failed\n");
				exit(1);
			}
			// printf("write ok: %d\n", i);
		}
		close(fd0[1]);
		exit(0);
	}

	int fd1[2], fd2[2];

	fd1[0] = fd0[0];
	fd1[1] = fd0[1];

	int child_flag = 0;

	int p;
	do {
		child_flag = 0;

		// 筛选完毕，不再fork新进程
		if (read(fd1[0], &p, sizeof(int)) != sizeof(int)) {
			close(fd1[0]);
			exit(0);
		}

		fprintf(1, "prime %d\n", p);

		// 创建 pipe2
		if (pipe(fd2) < 0) {
			fprintf(2, "primes: pipe failed\n");
			exit(1);
		}

		if (fork() == 0) {
			child_flag = 1;
			origin_flag = 0;
			// 将 fd2 赋值给 fd1	
			fd1[0] = fd2[0];
			fd1[1] = fd2[1];
		}

	} while (child_flag == 1);	


	int n;
	while (1) {
		if (read(fd1[0], &n, sizeof(int)) != sizeof(int) ) {
			close(fd2[1]);
			close(fd1[0]);
			break;
		}	
		if (n % p != 0) {
			if (write(fd2[1], &n, sizeof(int)) != sizeof(int)) {
				fprintf(2, "prime: pipe write failed\n");
				exit(1);
			}
		}
	}
	exit(0);
}
