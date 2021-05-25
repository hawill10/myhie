#include <stdio.h>
#include <string.h>
#include <stdbool.h> 
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/times.h>

#define READ_END 0
#define WRITE_END 1
#define BUFF_SIZE 255
#define NUM_COL 6

extern int** fd;
extern int time_fd[2];
extern int* ranges;

// option holder
extern char* input_file;
extern char* output_file;
extern int num_workers;
extern bool random_range;
extern int attribute_num;
extern bool attribute_set;
extern char order;

//coord
void parse(int argc, char* argv[]); // parse arguments
void create_pipes_wm(int** fd, int num_workers);    // create pipes between workers and merger
int compar (const void * a, const void * b);    // helper for qsort
void get_rand_range(int* ranges, int num_workers, int line_count); // create random range (-r)
void get_equal_range(int* ranges, int num_workers, int line_count); // create equal range
void get_range(int* ranges, int random_range, int num_workers, int line_count); // choose between random and equal range
void showReturnStatus(pid_t childpid, int status);  // show return status of merger node

void close_all_pipes(); // close read and write end of pipes used between the merger and workers

void free_fd(); // free pipes before exiting
void signal_handler(int sig);   // when merger send SIGUSR2, release dynamically allocated memory and send SIGUSR2 to root

//merger
void string_to_list(char** merged, char buff[BUFF_SIZE]);   // convert string (of each person) to array of strings
void read_sorted(int* file, char*** merged, int start, int end);    // read from worker
void quicksort_for_merge(char*** arr, int start, int end, int attribute);   // helper function used for merging
void merge_sorted_lists(char*** merged, int* ranges);   // merge partially sorted lists
void read_time_stat(char* time_stats[num_workers], int read_fd);    // read time statistics from workers

// worker
void write_msg(char*** data, int k, int len, int write_end);    // write sroted list to merger


