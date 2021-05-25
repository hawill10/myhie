#include "myhie.h"

int sigusr1_count = 0;
int sigusr2_count = 0;

void signal_handler(int sig) {
    signal(SIGUSR1, signal_handler);
    signal(SIGUSR2, signal_handler);
    
    if (sig==SIGUSR1) {
		sigusr1_count++;
	}
    if (sig==SIGUSR2) {
        sigusr2_count++;
    }
}

int  main(int argc, char* argv[]) {
    // argument format: ./myhie -i InputFile -k NumOfWorkers -r -a AttributeNumber -o Order -s OutputFile
    // for timing----------------------------------
    double t1, t2, cpu_time;
    struct tms tb1, tb2;
    double tics_per_sec;
    
    tics_per_sec = (double) sysconf(_SC_CLK_TCK);
    
    t1 = (double) times(&tb1);
    //----------------------------------------------

    signal(SIGUSR1, signal_handler);
    signal(SIGUSR2, signal_handler);

    int pid;
    if((pid = fork()) == 0) {
        //coord process
        argv[0] = "./coord";
        execv(argv[0], &argv[0]);
    }
    else {
        waitpid(pid, NULL, 0);

        printf("[Root] %d SIGUSR1 received\n", sigusr1_count);
        printf("[Root] %d SIGUSR2 received\n", sigusr2_count);
    }

    // print time statistics
    t2 = (double) times(&tb2);
    cpu_time = (double) ((tb2.tms_utime + tb2.tms_stime) - (tb1.tms_utime + tb1.tms_stime));
    
    printf("[Root] Run time was %1f sec (REAL time) although "  
            "we used the CPU for %1f sec (CPU time).\n", (t2 - t1)/tics_per_sec, cpu_time/tics_per_sec);
    //----------------------------------------------
    printf("[Root] exit\n");
}