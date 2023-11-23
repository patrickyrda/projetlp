#include <configuration.h>
#include <stddef.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdio.h>
#include <string.h>

typedef enum {DATE_SIZE_ONLY, NO_PARALLEL, DRY_RUN} long_opt_values;

/*!
 * @brief function display_help displays a brief manual for the program usage
 * @param my_name is the name of the binary file
 * This function is provided with its code, you don't have to implement nor modify it.
 */
void display_help(char *my_name) {
    printf("%s [options] source_dir destination_dir\n", my_name);
    printf("Options: \t-n <processes count>\tnumber of processes for file calculations\n");
    printf("         \t-h display help (this text)\n");
    printf("         \t--date_size_only disables MD5 calculation for files\n");
    printf("         \t--no-parallel disables parallel computing (cancels values of option -n)\n");
}

/*!
 * @brief init_configuration initializes the configuration with default values
 * @param the_config is a pointer to the configuration to be initialized
 */
void init_configuration(configuration_t *the_config) {
    strncpy(the_config->source, "", sizeof(the_config->source));
    the_config->source[sizeof(the_config->source) - 1] = '\0';
    strncpy(the_config->destination, "", sizeof(the_config->destination));
    the_config->destination[sizeof(the_config->destination) - 1] = '\0';
    the_config->processes_count = 1;        // !! check how many processes
    the_config->is_parallel = true;
    the_config->uses_md5 = true;
    the_config->is_verbose = false;
    the_config->is_dry_run = false;
}

/*!
 * @brief set_configuration updates a configuration based on options and parameters passed to the program CLI
 * @param the_config is a pointer to the configuration to update
 * @param argc is the number of arguments to be processed
 * @param argv is an array of strings with the program parameters
 * @return -1 if configuration cannot succeed, 0 when ok
 */
int set_configuration(configuration_t *the_config, int argc, char *argv[]) {
    int opt = 0;                         //!! is there an interest for using a counter
    static struct option my_opts[] = {                    //!! does it need to be static
    {.name="date-size-only",.has_arg=0,.flag=0,.val=DATE_SIZE_ONLY},
    {.name="no-parallel",.has_arg=0,.flag=0,.val=NO_PARALLEL},                  
    {.name="dry-run",.has_arg=0,.flag=0,.val=DRY_RUN},
    {.name=0,.has_arg=0,.flag=0,.val=0}, 
    };

    while ((opt = getopt_long(argc, argv, "n:vh", my_opts, NULL)) != -1) {
        switch (opt) {
            case 'n':
            the_config->processes_count = atoi(optarg);
            if (the_config->processes_count == 0) {
                fprintf(stderr, "Option should be a number!.\n");
                return -1;
            } else if (optarg == NULL) {
                fprintf(stderr, "Error: Option -n requires an argument.\n");
                return -1;
            } else {
            the_config->processes_count = atoi(optarg);
            }
            break;
            case 'v':
            the_config->is_verbose = true;
            break;
            case 'h':
            display_help(argv[0]);
            break;
            case DATE_SIZE_ONLY:
            the_config->uses_md5 = false;
            break;
            case NO_PARALLEL:
            the_config->is_parallel = false;
            break;
            case DRY_RUN:
            the_config->is_dry_run = true;
            //!! do something, do we need to do it later or add a function
            break;
            case '?':    
            if (optopt != 'n') {
                printf("Unknown option: %c\n", optopt); //should i stop the program when encoutering this or not, if not get rid cuz it takes n as error when it has no args
            }
            break;
            default: 
            printf("unexpected case!\n"); //what about this one, it shows 
        
        }   
    }
    
    if (argc - optind != 2) {
        fprintf(stderr, "Error: Please provide both source and destination directories.\n");
        return -1;
    }

    //  !!Copy the source and destination directories into the configuration, need to make different controls for when have more or less than 2 directorys
    strncpy(the_config->source, argv[optind], sizeof(the_config->source) - 1);
    the_config->source[sizeof(the_config->source) - 1] = '\0';

    strncpy(the_config->destination, argv[optind + 1], sizeof(the_config->destination) - 1);
    the_config->destination[sizeof(the_config->destination) - 1] = '\0';

    return 0;


}
