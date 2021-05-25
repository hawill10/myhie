#include <time.h>

#include "coord_func.h"

// variables to hold options
char* input_file;
char* output_file;
int num_workers;
bool random_range;
int attribute_num;
bool attribute_set;
char order;

int* ranges;
int** fd;   // hold pipes
int time_fd[2];

// parsing arguments: https://www.geeksforgeeks.org/getopt-function-in-c-to-parse-command-line-arguments/
void parse(int argc, char* argv[]){
    char opt;
    random_range = false;
    attribute_set = false;
    while((opt = getopt(argc, argv, ":i:k:ra:o:s:")) != -1) {

        switch(opt)
        {
            case 'i':
                input_file = optarg;
                // printf("input file: %s\n", optarg);
                break;
            case 'k':
                num_workers = atoi(optarg);
                // printf("numWorkers: %d\n", atoi(optarg));
                break;
            case 'r':
                random_range = true;
                // printf("flag %c: do random\n", opt);
                break;
            case 'a':
                attribute_num = atoi(optarg);
                attribute_set = true;
                // printf("Attribute Number: %d\n", atoi(optarg));
                break;
            case 'o':
                order = optarg[0];
                // printf("Order: %s\n", optarg);
                break;
            case 's':
                output_file = optarg;
                // printf("output file: %s\n", optarg);
                break;
            case ':':
                fprintf(stderr, "Error: ./myhie -i InputFile -k NumOfWorkers -r -a AttributeNumber -o Order -s OutputFile\n");  // merger fork failed
                exit(EXIT_FAILURE);
            case '?':
                fprintf(stderr, "Error: ./myhie -i InputFile -k NumOfWorkers -r -a AttributeNumber -o Order -s OutputFile\n");  // merger fork failed
                exit(EXIT_FAILURE);
        }
    }

    if ((optind < argc) || !(input_file && num_workers && attribute_set && order && output_file)) {
        fprintf(stderr, "Error: ./myhie -i InputFile -k NumOfWorkers -r -a AttributeNumber -o Order -s OutputFile\n");  // merger fork failed
        exit(EXIT_FAILURE);
    }
}

void showReturnStatus(pid_t childpid, int status) {
    if (WIFEXITED(status) && !WEXITSTATUS(status))
        printf("[Coord] Merger %ld terminated normally\n", (long)childpid);
    else if (WIFEXITED(status))
        printf("[Coord] Merger %ld terminated with return status %d\n", (long)childpid, WEXITSTATUS(status));
    else if (WIFSIGNALED(status))
        printf("[Coord] Merger %ld terminated due to uncaught signal %d\n", (long)childpid, WTERMSIG(status));
    else if (WIFSTOPPED(status))
        printf("[Coord] Merger %ld stopped due to signal %d\n", (long)childpid, WSTOPSIG(status));
}

void create_pipes_wm(int** fd, int num_workers) {
    for(int k=0; k < num_workers; k++){
        fd[k] = malloc(sizeof(int)*2);
        if (pipe(fd[k]) == -1) {
            fprintf(stderr, "failed to create a pipe\n");
            exit(EXIT_FAILURE);
        }
    }
}
void string_to_list(char** record, char buff[BUFF_SIZE]){
    char* token;
    token = strtok(buff, " ");
    record[0] = malloc(strlen(token));
    strcpy(record[0], token);
    for (int i = 1; i < NUM_COL; i++) {
        token = strtok(NULL, " ");
        record[i] = malloc(strlen(token));
        strcpy(record[i], token);
    }
    // reset index and buff for next message
    memset(buff, 0, strlen(buff));
}

void read_sorted(int* file, char*** merged, int start, int end) {
    close(file[WRITE_END]);    //close write_end for all pipe

    char buff[BUFF_SIZE];
    char tmp;
    int index = 0;
    ssize_t read_in;

    while( start < end && (read_in = read(file[READ_END], &tmp, 1)) > 0 ) {
        // end of message, so print it as whole
        if (tmp == '\0') {
            buff[index] = tmp;
            merged[start] = malloc(NUM_COL*sizeof(char*));
            string_to_list(merged[start], buff);
            index = 0;
            start++;
        }
        else {
            // add char to message
            buff[index] = tmp;
            index++;
        }
    }
    close(file[READ_END]);
}

void close_all_pipes(){
    for(int k=0; k < num_workers; k++){
            close(fd[k][WRITE_END]);
            close(fd[k][READ_END]);
    }
}

void write_msg(char*** data, int k, int len, int write_end) {
    for (int i=0; i < len; i++){
        //create message
        for(int c=0; c < NUM_COL-1; c++) {
            write(write_end, data[i][c], strlen(data[i][c]));
            write(write_end, " ", 1);
        }
        write(write_end, data[i][NUM_COL-1], strlen(data[i][NUM_COL-1])+1);
    }

    // Close write end after finished writing
    close(write_end);
}

void free_fd() {
    for(int k=0; k < num_workers; k++) {
        free(fd[k]);
    }
    free(fd);
}

// function used for qsort
int compar (const void * a, const void * b) {
    return ( *(int*)a - *(int*)b );
}

void get_rand_range(int* ranges, int num_workers, int line_count) {
    //divide lines into random ranges
    int index = 0;
    ranges[num_workers-1] = 0;
    ranges[num_workers] = line_count;
    srand(time(0));

    int range;
    do {
        range = rand()%line_count;
    }
    while(range==0);
    ranges[index] = range;
    
    index++;
    bool found;

    while(index < num_workers-1) {
        do {
            found = false;
            range = rand()%line_count;
            for (int i=0; i < index; i++){
                if (ranges[i] == range) {
                    found = true;
                }
            }
        }
        while(range==0 || found);
        ranges[index] = range;
    
        index++;
    }

    // http://www.cplusplus.com/reference/cstdlib/qsort/
    qsort(ranges, num_workers+1, sizeof(int), compar);
}

void get_equal_range(int* ranges, int num_workers, int line_count) {
    // divide lines into equal sized ranges
    int each_group = line_count/num_workers;
    int remains = line_count%num_workers;
    int* tmp_ranges = malloc(sizeof(int)*num_workers);

    for(int i=0; i < num_workers; i++) {
        tmp_ranges[i] = each_group;
    }
    
    int index = 0;
    while(remains > 0) {
        tmp_ranges[index]++;
        index = (index+1)%num_workers;
        remains--;
    }
    
    ranges[0] = 0;
    int temp = 0;
    for(int i=0; i<num_workers; i++){
        temp += tmp_ranges[i];
        ranges[i+1] = temp;
    }

    free(tmp_ranges);
}

void get_range(int* ranges, int random_range, int num_workers, int line_count) {
    if (num_workers == 1) {
        ranges[0] = 0; 
        ranges[1] = line_count;  // if only 1 worker
        return;
    }
    if (random_range) {
        get_rand_range(ranges, num_workers, line_count);
    }
    else {
        get_equal_range(ranges, num_workers, line_count);
    }  
}

void quicksort_for_merge(char*** arr, int start, int end, int attribute) {
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
        quicksort_for_merge(arr, start, right-1, attribute);
    }
    if (end > right + 1){
        quicksort_for_merge(arr, right + 1, end, attribute);
    }  
}


void merge_sorted_lists(char*** merged, int* ranges){
    int num_blocks = num_workers;
    int* tmp_r = malloc(sizeof(int)*(num_workers + 1));
    tmp_r = ranges;
    int step = 1;
    while(num_blocks > 1) {
        printf("[Merger] Merging in progress (%d steps completed)\n", step);
        int next_num_blocks = (num_blocks%2 ? num_blocks/2 + 1 : num_blocks/2);
        int* new_r = malloc(sizeof(int)*(next_num_blocks+1));
        new_r[0] = 0;
        int ind = 1;
        
        for (int g=0; g < num_blocks/2; g++) {
            quicksort_for_merge(merged, tmp_r[2*g], tmp_r[2*g+2]-1, attribute_num);
            new_r[ind] = tmp_r[2*g+2];
            ind++;
        }
        if (num_blocks%2) {
            new_r[next_num_blocks] = tmp_r[num_blocks];
        }
        free(tmp_r);
        tmp_r = new_r;

        num_blocks = next_num_blocks;
        step++;
    }
    free(tmp_r);
}

void signal_handler(int sig) {
    signal(SIGUSR2, signal_handler);
    
    if (sig==SIGUSR2) {
        // free dynamically allocated memory
        free_fd();
        free(ranges);
        // send SIGUSR2 to root
        kill(getppid(), SIGUSR2);
    }
}

void read_time_stat(char* time_stats[num_workers], int read_fd){
    char buff[BUFF_SIZE];
    char tmp;
    int index = 0;
    ssize_t read_in;
    int stat_i = 0;

    while( (read_in = read(read_fd, &tmp, 1)) > 0 ) {
        // end of message, so print it as whole
        if (tmp == '\0') {
            buff[index] = tmp;
            time_stats[stat_i] = malloc(sizeof(char)*(strlen(buff) + 1));
            strcpy(time_stats[stat_i], buff);
            memset(buff, 0, strlen(buff));
            index = 0;
            stat_i++;
        }
        else {
            // add char to message
            buff[index] = tmp;
            index++;
        }
    }
}

