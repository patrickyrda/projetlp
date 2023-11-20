#ifndef OPTIONS_H
#define OPTIONS_H

// Structure pour stocker les options du programme
typedef struct {
    int date_size_only;
    int num_processes;
    int no_parallel;
    int verbose;
    int dry_run;
} program_options;

void parse_options(int argc, char **argv);
program_options get_program_options();

#endif // OPTIONS_H
