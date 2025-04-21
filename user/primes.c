#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define DEBUG(x) printf("this is %d dubug message.\n", x);

int count = 0;
void primes(int fd[]) {

	close(fd[1]);

	int rd = fd[0];
	int wr;

		int next_fd[2];

	int p;
	int read_bytes = read(rd, &p, sizeof(int));

	if (read_bytes == sizeof(int)) {
		if ( pipe(next_fd) < 0) {
			fprintf(2, "primes: pipe failed\n");
			exit(1);	
		}
		wr = next_fd[1];

		printf("prime %d\n", p); // 输出素数 p
	}
	else { // 末尾流
		close(rd);
		exit(0);
	}

	if (fork() != 0) {
		close(rd);
		primes(next_fd);
		return;
	}
	else {
		close(next_fd[0]);

		while (1) {
			int n;
			int read_bytes = read(rd, &n, sizeof(int));
	
			if (read_bytes != sizeof(int)) {
				close(rd);
				close(wr);
				break;
			}
			
			if (n % p != 0) {
				int write_bytes = write(wr, &n, sizeof(int));
				if (write_bytes != sizeof(int)) {
					fprintf(2, "primes: pipe write failed\n");
					exit(1);
				}
			}
		}

		wait(0);
		
	}
	return;

}

int main (int argc, char *argv[]) {
	if (argc != 1) {
		fprintf(2, "Usage: primes\n");
		exit(1);
	}	

	int fd[2];

	if ( pipe(fd) < 0) {
		fprintf(2, "primes: pipe failed\n");
		exit(1);	
	}

	if (fork() == 0) {
		primes(fd);
	}
	else {
		close(fd[0]);
		for (int i = 2; i <= 280; i ++) {
			int write_bytes = write(fd[1], &i, sizeof(int));
			if (write_bytes != sizeof(int)) {
				fprintf(2, "primes: pipe write failed\n");
				exit(1);
			}
		}
		close(fd[1]);
		wait(0);
	}

	exit(0);

}
