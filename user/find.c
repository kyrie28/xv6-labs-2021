#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

#define MAXPATHLEN 512

void rec(char *path, char *filename) {
	char *p = path + strlen(path);
	int fd;
	struct dirent de;
	struct stat st;

	if ((fd = open(path, 0)) < 0) {
		fprintf(2, "find: cannot open %s\n", path);
    	return;
  	}

	if (fstat(fd, &st) < 0) {
	    fprintf(2, "find: cannot stat %s\n", path);
	    close(fd);
    	return;
  	}

	if (st.type != T_DIR) {
		fprintf(2, "find: %s is not directory\n", path);
		close(fd);
		return;
	}

	if (strlen(path) + 1 + DIRSIZ + 1 > MAXPATHLEN) {
    	printf("ls: path too long\n");
    	close(fd);
		return;
    }
	*p++ = '/';
	while (read(fd, &de, sizeof(de)) == sizeof(de)) {
		if (de.inum == 0)
			continue;
		memmove(p, de.name, DIRSIZ); 
		p[DIRSIZ] = 0;
		if (stat(path, &st) < 0) {
       		printf("ls: cannot stat %s\n", path);
        	continue;
      	}
		if (strcmp(de.name, filename) == 0)
			printf("%s\n", path);
		if (strcmp(de.name, ".") != 0 && strcmp(de.name, "..") != 0 && st.type == T_DIR)
			rec(path, filename);
	}	

	close(fd);
}

int main(int argc, char *argv[]) {
	char buf[MAXPATHLEN];
	strcpy(buf, argv[1]);
	rec(buf, argv[2]);
	exit(0);
}
