#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "coord_func.h"
#include "file_ops.h"

void quicksort(char*** arr, int start, int end, int attribute) {
    int left = start;
    int right = end;
    double pivot = atof(arr[end][attribute]);

    while(right > left) {
        if (atof(arr[left][attribute]) >= pivot) {
            char** tmp_ptr = arr[right];
            arr[right] = arr[left];
            arr[left] = arr[right-1];
            arr[right-1] = tmp_ptr;
            right--;
        }
        else {
            left++;
        }
    }

    if (right > start){
        quicksort(arr, start, right-1, attribute);
    }
    if (end > right + 1){
        quicksort(arr, right + 1, end, attribute);
    }  
}

int main(int argc, char* argv[]) {
    // for timing----------------------------------
    double t1, t2, cpu_time;
    struct tms tb1, tb2;
    double tics_per_sec;
    
    tics_per_sec = (double) sysconf(_SC_CLK_TCK);
    
    t1 = (double) times(&tb1);
    //----------------------------------------------

    int k = atoi(argv[1]);
    char* input_file = argv[2];
    long int jump_to = atoi(argv[3]);
    int count = atoi(argv[4]);
    int attribute_num = atoi(argv[5]);
    int write_end = atoi(argv[6]);
    int time_write = atoi(argv[7]);

    char*** data = read_file_range(input_file, jump_to, count); // read file
    printf("[Worker %d] Started sorting...\n", k);
    quicksort(data, 0, count -1, attribute_num);    //sort data
    // write messages to pipe
    printf("[Worker %d] Started writing to merger...\n", k);
    write_msg(data, k, count, write_end);

    t2 = (double) times(&tb2);
    cpu_time = (double) ((tb2.tms_utime + tb2.tms_stime) - (tb1.tms_utime + tb1.tms_stime));

    char msg[BUFF_SIZE];
    sprintf(msg, "[Worker %d (Quick Sort)] Run time was %1f sec (REAL time) although we used the CPU for %1f sec (CPU time).", k, (t2 - t1)/tics_per_sec, cpu_time/tics_per_sec);

    write(time_write, msg, strlen(msg) + 1);
    exit(0);
}