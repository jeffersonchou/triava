
#include <stdio.h>
#include <stdlib.h>


void memusage()
{
    char line[128];

    FILE* status = fopen("/proc/self/status", "r");
    if (!status)
    {
        printf("error: unable to open /proc/self/status.\n");
        return;
    }

    long long unsigned VmRSS = 0;
    
    while (!feof(status)) 
    {
        fgets(line, 128, status);
        if (strncmp(line, "VmRSS:", 6) == 0)
        {
            int n = sscanf(line, "VmRSS: ""%llu", &VmRSS);
            if (n != 1) 
            {
                printf("error: could not parse VmRSS\n");
            }
            break;
        }
    }
    fclose(status);

    printf("[memory usage] %llu.\n",VmRSS);
}

int main(int argc, char **argv)
{
	memusage();
	while (1)
	{
     
	  sleep(2);
          memusage();
        }
	return 0;
}
