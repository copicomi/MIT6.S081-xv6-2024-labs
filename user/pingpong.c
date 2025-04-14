#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main (int argc, char *argv[]) {
	
	if (argc != 1) {
		fprintf(2, "Usage: pingpong\n");
		exit(1);
	}

	int fd_p2c[2];
	int fd_c2p[2];

	if (pipe(fd_p2c) < 0 || pipe(fd_c2p) < 0) {
		fprintf(2, "pingpong: pipe failed\n");
		exit(1);
	}

	int pid = fork();

	if (pid < 0) {
		fprintf(2, "pingpong: fork failed\n");
		exit(1);
	}
	else if (pid == 0) {
		// child
		close(fd_p2c[1]);
		close(fd_c2p[0]);

		char c;
		if (read(fd_p2c[0], &c, 1) != 1) {
			fprintf(2, "pingpong: pipe read failed\n");
			exit(1);
		}

		fprintf(1, "%d: received ping\n", getpid());	

		if (write(fd_c2p[1], "L", 1) != 1) {
			fprintf(2, "pingpong: pipe write failed\n");
			exit(1);
		}

		close(fd_p2c[0]);
		close(fd_c2p[1]);
		
		exit(0);
	}
	else {
		// parent
		close(fd_p2c[0]);
		close(fd_c2p[1]);

		if (write(fd_p2c[1], "L", 1) != 1) {
			fprintf(2, "pingpong: pipe write failed\n");
			exit(1);
		}

		char c;
		if (read(fd_c2p[0], &c, 1) != 1) {
			fprintf(2, "pingpong: pipe read failed\n");
			exit(1);
		}

		fprintf(1, "%d: received pong\n", getpid());	

		close(fd_p2c[1]);
		close(fd_c2p[0]);

		exit(0);
	}

}
