#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

int fork1(void) {
	int pid;

	pid = fork();

	if (pid == -1) {
		fprintf(2, "xargs: fork failed\n");
		exit(1);
	}
	return pid;
}

void debug(int argc_, char *argv_[]) {

					for (int i = 0; i < argc_; i ++) {
						printf("%s ", argv_[i]);

					}
					printf("\n");
}

int main (int argc, char *argv[]) {
	if (argc > MAXARG + 1) {
		fprintf(2, "xargs: too many args\n");
		exit(1);
	}

	char *argv_[MAXARG] = { 0 };

	for (int i = 0; i < argc - 1; i ++) {
		argv_[i] = argv[i + 1];
	}


	char p;
	char buf[255];
	char *rd_argv = buf;
	int argc_ = argc - 1;
	int reading_argv = 0;
	char *arg_begin = rd_argv;


	while (read(0, &p, sizeof(char)) == sizeof(char)) {

		switch (p) {
			case ' ':
			case '\n':
				if (reading_argv == 1) { // 更新指针位置
					if (argc_ > MAXARG) {
						fprintf(2, "xargs: too many args\n");
						exit(1);
					}
	
					*(rd_argv ++) = '\0'; // 字符串结尾
					argv_[argc_ ++] = arg_begin;

					reading_argv = 0;
				}

				if (p == '\n') {
					if (fork1() == 0) {
						exec(argv_[0], argv_);
						fprintf(2, "xargs: exec %s failed\n", argv_[0]);
						exit(1);
					}
					// debug(argc_, argv_);
					wait(0);
					rd_argv = buf;
					argc_ = argc - 1;
				}
				arg_begin = rd_argv;
				break;

			default:
				if (rd_argv >= buf + 255) {
					fprintf(2, "xargs: args too long\n");
					exit(1);
				}
				*(rd_argv ++) = p;
				reading_argv = 1;
				break;
		}
	}
	exit(0);

}
