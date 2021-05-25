#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define NUM_COL 6
#define BUFF_SIZE 255


int count_lines(char* filename) {
    FILE* data_file;

    data_file = fopen(filename, "r");

    if (!data_file) {
        fprintf(stderr, "File '%s' doesn't exist\n", filename);
        exit(EXIT_FAILURE);
    }   

    char* line = NULL;
    size_t len = 0;
    int line_count = 0;

    while(getline(&line, &len, data_file) != -1) {
        line_count++;
    }

    return line_count;
}

char*** read_file_range(char* filename, int jump_to, int count) {
    //read file
    FILE* data_file;

    data_file = fopen(filename, "r");

    if (!data_file) {
        printf("File '%s' doesn't exist\n", filename);
        return 0;
    }

    // move to jump_to
    fseek(data_file, jump_to, SEEK_SET);

    // References:
    // https://stackoverflow.com/questions/3501338/c-read-file-line-by-line
    // https://stackoverflow.com/questions/31057175/reading-text-file-of-unknown-size/31064934
    char* line = NULL;
    size_t len = 0;
    ssize_t read;

    int line_num = 0;
    size_t size = count;
    char*** data = malloc(sizeof(line)*size);
    if(data == NULL) {
        printf("out of memory\n");
        exit(1);
    }

    while((read = getline(&line, &len, data_file)) != -1 && line_num < count) {
        line[strcspn(line, "\n")] = 0;

        int num_col = 6;
        char* token;
        data[line_num] = malloc(num_col*sizeof(line));

        token = strtok(line, " ");
        data[line_num][0] = malloc(strlen(token));
        strcpy(data[line_num][0], token);
        for (int i = 1; i < num_col; i++) {
            token = strtok(NULL, " ");
            data[line_num][i] = malloc(strlen(token));
            strcpy(data[line_num][i], token);
        }
        
        line_num++;
    }
    
    fclose(data_file);

    return data;
}

long int get_file_position(FILE* data_file, int count) {
    char* line = NULL;
    size_t len = 0;
    ssize_t read;

    int line_num = 1;
    // read till the point where next worker should start reading from
    while((read = getline(&line, &len, data_file)) != -1 && line_num != count) {
        line_num++;
    }

    return ftell(data_file);
}

void fwrite_sorted(char* output_file, char*** merged, int line_count, char order){
    FILE* output;
    output = fopen(output_file, "w");
    if (order == 'a'){
        for(int i=0; i<line_count; i++){
            for(int j=0; j < 6; j++){
                fputs(merged[i][j], output);
                if(j != 5){
                    fputc(' ', output);
                }
            }
            fputc('\n', output);
        }
    }
    else {
        for(int i=line_count-1; i>=0; i--){
            for(int j=0; j < 6; j++){
                fputs(merged[i][j], output);
                if(j != 5){
                    fputc(' ', output);
                }
            }
            fputc('\n', output);
        }
    }
    fclose(output);
}