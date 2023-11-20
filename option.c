#include "option.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

// Initialisation des options par défaut
program_options options = {0, 1, 0, 0, 0}; // Valeurs par défaut

// Fonction pour analyser les arguments de ligne de commande
void parse_options(int argc, char **argv) {
    int opt;
    static struct option long_options[] = {
            {"date-size-only", no_argument, 0, 'd'},
            {"n", required_argument, 0, 'n'},
            {"no-parallel", no_argument, 0, 'p'},
            {"verbose", no_argument, 0, 'v'},
            {"dry-run", no_argument, 0, 'r'},
            {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, argv, "dn:pvr", long_options, NULL)) != -1) {
        switch (opt) {
            case 'd':
                options.date_size_only = 1;
                break;
            case 'n':
                options.num_processes = atoi(optarg);
                break;
            case 'p':
                options.no_parallel = 1;
                break;
            case 'v':
                options.verbose = 1;
                break;
            case 'r':
                options.dry_run = 1;
                break;
            default:
                fprintf(stderr, "Usage: %s [-d] [-n <num>] [-p] [-v] [-r]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }
}

// Fonction pour obtenir les options du programme
program_options get_program_options() {
    return options;
}
