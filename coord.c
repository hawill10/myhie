#include "coord_func.h"
#include "file_ops.h"


int main(int argc, char* argv[]) {
    // for timing----------------------------------
    double t1, t2, cpu_time;
    struct tms tb1, tb2;
    double tics_per_sec;
    
    tics_per_sec = (double) sysconf(_SC_CLK_TCK);
    
    t1 = (double) times(&tb1);
    //----------------------------------------------
    parse(argc, argv);

    // test if arguments are in
    // printf("-----------------------------------\n");
    // printf("input file: %s\n", input_file);
    // printf("output file: %s\n", output_file);
    // printf("num workers: %d\n", num_workers);
    // printf("random range: %d\n", random_range);
    // printf("attribute num: %d\n", attribute_num);
    // printf("order: %c\n", order);
    // printf("-----------------------------------\n");

    // root id
    int root_id = getppid();

    int line_count = count_lines(input_file);   // count line of total file, to create range
    int* ranges = malloc(sizeof(int)*(num_workers+1));
    
    get_range(ranges, random_range, num_workers, line_count); // create range for sorting
    // test: print ranges
    // for(int i =0; i<num_workers+1; i++) {    
    //     printf("%d ", ranges[i]);
    // }
    // printf("\n");

    //create pipe for each worker
    fd = malloc(sizeof(int *)*num_workers);
    create_pipes_wm(fd, num_workers);

    // pipe for time stat
    int time_fd[2];
    if (pipe(time_fd) == -1) {
        fprintf(stderr, "failed to create a pipe\n");
        exit(EXIT_FAILURE);
    }

    int merger = fork();    // create merger

    if (merger < 0){    
        fprintf(stderr, "[Coord] failed to fork merger\n");  // merger fork failed
        exit(EXIT_FAILURE);
    }
    
    if (merger == 0) {
        // merger
        t1 = (double) times(&tb1);  //time merger
        
        close(time_fd[WRITE_END]);

        char*** merged;
        merged = malloc(sizeof(merged)*line_count);

        printf("[Merger] Start reading from workers...\n");
        for (int k=0; k < num_workers; k++){
            printf("[Merger] Reading from worker %d...\n", k);
            read_sorted(fd[k], merged, ranges[k], ranges[k+1]);    // keep reading from pipe so it is not blocked;
        }
        
        // read times
        char* time_stats[num_workers];
        read_time_stat(time_stats, time_fd[READ_END]);

        printf("[Merger] Merging partially sorted lists...\n");
        // merge list of sorted lists (merged)
        merge_sorted_lists(merged, ranges);
        printf("[Merger] Merging done\n");

        fwrite_sorted(output_file, merged, line_count, order);
        printf("[Merger] Output written to file '%s'!\n", output_file);

        // free merged
        for (int i=0; i < line_count; i++) {
            for (int j=0; j < NUM_COL; j++){
                free(merged[i][j]);
            }
        }

        t2 = (double) times(&tb2);
        cpu_time = (double) ((tb2.tms_utime + tb2.tms_stime) - (tb1.tms_utime + tb1.tms_stime));

        for (int k=0; k < num_workers; k++){
            printf("%s\n", time_stats[k]);
        }
        
        printf("[Merger] Run time was %1f sec (REAL time) although "  
                "we used the CPU for %1f sec (CPU time).\n", (t2 - t1)/tics_per_sec, cpu_time/tics_per_sec);
        // send USR2 to coord
        kill(getppid(), SIGUSR2);
        exit(0);
    }

    // OUT of MERGER: Back to COORD (Start Making Workers)
    FILE* file;

    file = fopen(input_file, "r");
    long int jump_to = 0;

    for (int k = 0; k < num_workers; k++){
        int worker = fork();

        if (worker < 0) {
            fprintf(stderr, "[Coord] failed to fork worker %d\n", k);
            exit(EXIT_FAILURE);
        }

        if (worker == 0) {
            //worker
            fclose(file);
            // close irrelevant pipes
            close(time_fd[READ_END]);

            // close read end
            close(fd[k][READ_END]);
            // close write end except the one writing
            for(int i=0; i < num_workers; i++) {
                if(i != k) {
                    close(fd[i][WRITE_END]);
                }
            }
            int sorter;
            if ( (sorter = fork()) == 0 ){
                // run sort
                // convert int to string
                char sort_type[20];
                sprintf(sort_type, "%s", (k%2 ? "./custom_mergesort" : "./quicksort"));
                char worker_num[10];
                sprintf(worker_num, "%d", k);
                char jump_to_str[20];
                sprintf(jump_to_str, "%ld", jump_to);
                char count[20];
                sprintf(count, "%d", ranges[k+1] - ranges[k]);
                char attribute[2];
                sprintf(attribute, "%d", attribute_num);
                char write_end[10];
                sprintf(write_end, "%d", fd[k][WRITE_END]);
                char time_write[10];
                sprintf(time_write, "%d", time_fd[WRITE_END]);
                
                execl(sort_type, sort_type, worker_num, input_file, jump_to_str, count, attribute, write_end, time_write, (char*) NULL);
            }
            else {
                close(time_fd[WRITE_END]);
                waitpid(sorter, NULL, 0);   // wait till sorter is finished
            }

            kill(root_id, SIGUSR1); // send SIGUSR1 to root
            printf("[Worker %d] Exit\n", k);
            exit(0);
        }
        // wait for workers without hang
        else {
            if (k < num_workers-1){
                jump_to = get_file_position(file, ranges[k+1] - ranges[k]);
            }
            pid_t pid;
            int status = 0;
            pid = waitpid(worker, &status, WNOHANG); //WNOHANG so workers can run parallel
        }
    }
    
    // OUTSIDE LOOP: Back to COORD
    // COORD doesn't read or write
    signal(SIGUSR2, signal_handler);

    fclose(file);

    printf("[Coord] Close all pipes\n");
    close_all_pipes();
    close(time_fd[READ_END]);
    close(time_fd[WRITE_END]);

    // wait for merger to finish
    printf("[Coord] Waiting for merger to finish...\n");
    int status = 0;
    int pid = waitpid(merger, &status, 0);
    showReturnStatus(pid, status);
    
    // print time statistics
    t2 = (double) times(&tb2);
    cpu_time = (double) ((tb2.tms_utime + tb2.tms_stime) - (tb1.tms_utime + tb1.tms_stime));
    
    printf("[Coord] Run time was %1f sec (REAL time) although "  
            "we used the CPU for %1f sec (CPU time).\n", (t2 - t1)/tics_per_sec, cpu_time/tics_per_sec);
    //----------------------------------------------
}