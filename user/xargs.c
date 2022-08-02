#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

int readline(int fd, char *buf) {
	while (read(fd, buf, 1) == 1) {
		if (*buf == '\n') {
			*buf = 0;
			return 1;
		}
		buf++;
	}
	*buf = 0;
	return 0;
}

int main(int argc, char *argv[]) {
	char buf[512];
	int count = 0;

	while (1) {
		int go = readline(0, buf);
		
		if (strlen(buf) != 0) {
			char **new_argv = malloc((argc + 1) * sizeof(char *));
			int i;
			for (i = 0; i < argc - 1; i++) {
				new_argv[i] = malloc(strlen(argv[i + 1]) + 1);
				memmove(new_argv[i], argv[i + 1], strlen(argv[i + 1]));
				new_argv[i][strlen(argv[i + 1])] = 0;
			}
			new_argv[argc - 1] = malloc(strlen(buf) + 1);
			memmove(new_argv[argc - 1], buf, strlen(buf));
			new_argv[argc - 1][strlen(buf)] = 0;
			new_argv[argc] = 0;

			if (fork() == 0) {
				exec(new_argv[0], new_argv);
				fprintf(2, "xargs: exec %s failed\n", argv[1]);
				exit(0);
			}

			count++;
		}

		if (!go)
			break;
	}

	for (; count > 0; count--)
		wait(0);

	exit(0);
}
