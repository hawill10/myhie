#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/times.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

void signal_handler(int sig); //increment count of signals received from workers (SIGUSR1) and coord (SIGUSR2)