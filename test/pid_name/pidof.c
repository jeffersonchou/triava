#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <dirent.h>
#include <stdlib.h>


#define READ_BUF_SIZE	50

static void *xrealloc(void *ptr, size_t size)
{
	ptr = realloc(ptr, size);
	if (ptr == NULL && size != 0)
		printf("error: memory_exhausted.\n");
	return ptr;
}


static long* find_pid_by_name( char* pidName)
{
	DIR *dir;
	struct dirent *next;
	long* pidList=NULL;
	int i=0;

	dir = opendir("/proc");
	if (!dir)
		printf("Cannot open /proc");
	
	while ((next = readdir(dir)) != NULL) {
		FILE *status;
		char filename[READ_BUF_SIZE];
		char buffer[READ_BUF_SIZE];
		char name[READ_BUF_SIZE];

		/* Must skip ".." since that is outside /proc */
		if (strcmp(next->d_name, "..") == 0)
			continue;

		/* If it isn't a number, we don't want it */
		if (!isdigit(*next->d_name))
			continue;

		sprintf(filename, "/proc/%s/status", next->d_name);
		if (! (status = fopen(filename, "r")) ) {
			continue;
		}
		if (fgets(buffer, READ_BUF_SIZE-1, status) == NULL) {
			fclose(status);
			continue;
		}
		fclose(status);

		/* Buffer should contain a string like "Name:   binary_name" */
		sscanf(buffer, "%*s %s", name);
		if (strcmp(name, pidName) == 0) {
			pidList=xrealloc( pidList, sizeof(long) * (i+2));
			pidList[i++]=strtol(next->d_name, NULL, 0);
		}
	}

	if (pidList) {
		pidList[i]=0;
	} else {
		pidList=xrealloc( pidList, sizeof(long));
		pidList[0]=-1;
	}
	return pidList;
}


int main(int argc, char **argv)
{
    char *name;
    long* pid_list;
    int n = 0;

    if (argc < 2)
        printf("pidof name");

    name = argv[1];
    pid_list = find_pid_by_name(name);

    for(; pid_list && *pid_list!=0; pid_list++) {
        printf("%s%ld\n", (n++ ? " " : ""), (long)*pid_list);
    }
    return 0;
}


