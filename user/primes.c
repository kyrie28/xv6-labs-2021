#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void create_one(int fd) {
	int prime;
    if (read(fd, &prime, 4) != 0) {
		if (prime == 0) {
			close(fd);
			return;
		}
        printf("prime %d\n", prime);
    
		int num;
		int pp[2];
		pipe(pp);
		if (fork() == 0) {
			close(pp[1]);
			create_one(pp[0]);
		} else {
			close(pp[0]);
			while (read(fd, &num, 4) != 0) {
				if (num == 0) {
					write(pp[1], &num, 4);
					close(pp[1]);
					break;
				}
				if (num % prime != 0) {
					write(pp[1], &num, 4);
				}
			}
			int status;
			wait(&status);
		}
    }
}

int main(int argc, char *argv[]) {
	int i;
	int pp[2];
	pipe(pp);
	for (i = 2; i <= 35; i++) {
		write(pp[1], &i, 4);
	}

	i = 0;
	write(pp[1], &i, 4);	
	create_one(pp[0]);
	exit(0);
}
