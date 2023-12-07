#include <sync.h>
#include <dirent.h>
#include <string.h>
#include <processes.h>
#include <utility.h>
#include <messages.h>
#include <file-properties.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <unistd.h>
#include <sys/msg.h>

#include <stdio.h>

/*!
 * @brief synchronize is the main function for synchronization
 * It will build the lists (source and destination), then make a third list with differences, and apply differences to the destination
 * It must adapt to the parallel or not operation of the program.
 * @param the_config is a pointer to the configuration
 * @param p_context is a pointer to the processes context
 */
void synchronize(configuration_t *the_config, process_context_t *p_context) {
}

/*!
 * @brief mismatch tests if two files with the same name (one in source, one in destination) are equal
 * @param lhd a files list entry from the source
 * @param rhd a files list entry from the destination
 * @has_md5 a value to enable or disable MD5 sum check
 * @return true if both files are not equal, false else
 */
bool mismatch(files_list_entry_t *lhd, files_list_entry_t *rhd, bool has_md5) {
}

/*!
 * @brief make_files_list buils a files list in no parallel mode
 * @param list is a pointer to the list that will be built
 * @param target_path is the path whose files to list
 */
void make_files_list(files_list_t *list, char *target_path) {
    if (list == NULL) {
        fprintf(stderr, "Error: Invalid pointer to files_list_t\n");
        return;
    }
    DIR *dir = opendir(target_path);
    if (dir == NULL) {
        perror("Error opening directory");
        return;
    }
    list->files = NULL;
    list->count = 0;
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Ignorer les entrées spéciales "." et ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        char *file_name = strdup(entry->d_name);
        if (file_name == NULL) {
            perror("Error allocating memory for file name");
            closedir(dir);
            return;
        }
        list->files = realloc(list->files, (list->count + 1) * sizeof(char *));
        if (list->files == NULL) {
            perror("Error reallocating memory for files list");
            free(file_name);
            closedir(dir);
            return;
        }
        list->files[list->count] = file_name;
        list->count++;
    }
    closedir(dir);
}

/*!
 * @brief make_files_lists_parallel makes both (src and dest) files list with parallel processing
 * @param src_list is a pointer to the source list to build
 * @param dst_list is a pointer to the destination list to build
 * @param the_config is a pointer to the program configuration
 * @param msg_queue is the id of the MQ used for communication
 */
void make_files_lists_parallel(files_list_t *src_list, files_list_t *dst_list, configuration_t *the_config, int msg_queue) {
}

/*!
 * @brief copy_entry_to_destination copies a file from the source to the destination
 * It keeps access modes and mtime (@see utimensat)
 * Pay attention to the path so that the prefixes are not repeated from the source to the destination
 * Use sendfile to copy the file, mkdir to create the directory
 */
void copy_entry_to_destination(files_list_entry_t *source_entry, configuration_t *the_config) {
}

/*!
 * @brief make_list lists files in a location (it recurses in directories)
 * It doesn't get files properties, only a list of paths
 * This function is used by make_files_list and make_files_list_parallel
 * @param list is a pointer to the list that will be built
 * @param target is the target dir whose content must be listed
 */
void make_list(files_list_t *list, char *target) {
    DIR *dir = open_dir(target);
    if (dir == NULL) {
        perror("Erreur");
        return;
    }
    struct dirent *entry;
    while ((entry = get_next_entry(dir)) != NULL) {
        char file_path[PATH_MAX];
        snprintf(file_path, PATH_MAX, "%s/%s", target, entry->d_name);
        add_file_entry(list, file_path);
        if (entry->d_type == DT_DIR) {
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                make_list(list, file_path);
            }
        }
    }
    closedir(dir);
}

/*!
 * @brief open_dir opens a dir
 * @param path is the path to the dir
 * @return a pointer to a dir, NULL if it cannot be opened
 */

DIR *open_dir(char *path) {
    DIR *dir = opendir(path); // Tentative d'ouverture du répertoire

    if (dir == NULL) {
        perror("Erreur lors de l'ouverture du répertoire"); // Affiche l'erreur en cas d'échec
    }

    return dir; // Renvoie le pointeur de DIR ou NULL si échec
}

/*!
 * @brief get_next_entry returns the next entry in an already opened dir
 * @param dir is a pointer to the dir (as a result of opendir, @see open_dir)
 * @return a struct dirent pointer to the next relevant entry, NULL if none found (use it to stop iterating)
 * Relevant entries are all regular files and dir, except . and ..
 */
struct dirent *get_next_entry(DIR *dir) {
    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {
        // Ignore les entrées '.' et '..'
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            return entry; // Renvoie l'entrée valide
        }
    }

    return NULL; // Fin du répertoire ou erreur
}
