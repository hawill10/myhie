int count_lines(char* filename);    // return total lines of a file (by coord)
long int get_file_position(FILE* data_file, int count); // return position after counting "count" number of lines (by coord)
char*** read_file_range(char* filename, int jump_to, int count);    // read "count" number of lines from "jump_to" (by worker)
void fwrite_sorted(char* output_file, char*** merged, int line_count, char order);  // write sorted array to output file (by merger)