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
    files_list_t *destination, *source, *differences;
    make_files_list(destination, the_config->destination);
    make_files_list(source, the_config->source);
    
    files_list_entry_t *destination_element = destination->head;

    while (destination_element) {
        
    }

}

/*!
 * @brief mismatch tests if two files with the same name (one in source, one in destination) are equal
 * @param lhd a files list entry from the source
 * @param rhd a files list entry from the destination
 * @has_md5 a value to enable or disable MD5 sum check
 * @return true if both files are not equal, false else
 */
bool mismatch(files_list_entry_t *lhd, files_list_entry_t *rhd, configuration_t *the_config) {             //im moving the bool has md5, might also move the name control!!
    //its an invalid input
    if (!lhd || !rhd) {
        return true;
    }

    //if we want to ignore name case sensitivity we can use the strlwr
    char *lhd_filename = get_file_name_from_path(lhd->path_and_name);
    char *rhd_filename = get_file_name_from_path(rhd->path_and_name);

    if (strcasecmp(lhd_filename, rhd_filename) != 0) {
        return true;
    }

    bool has_md5 = the_config->uses_md5;

    if (lhd->entry_type == FICHIER && rhd->entry_type == FICHIER) {

        if (has_md5) {
            if (lhd->entry_type != rhd->entry_type || memcmp(lhd->md5sum, rhd->md5sum, sizeof(lhd->md5sum)) != 0 || lhd->mtime.tv_sec != rhd->mtime.tv_sec || lhd->mtime.tv_nsec != rhd->mtime.tv_nsec || lhd->size != rhd->size) {
                return true;
            } else {
                return false;
            }
        } else {
            if (lhd->entry_type != rhd->entry_type || lhd->mtime.tv_sec != rhd->mtime.tv_sec || lhd->mtime.tv_nsec != rhd->mtime.tv_nsec || lhd->size != rhd->size) {
                return true;
            } else {
                return false;
            }
        }

    } else if (lhd->entry_type == DOSSIER && rhd->entry_type == DOSSIER) {
        //might need to add other conditions on this return 
        return lhd->mode != rhd->mode;
    }

    return true;

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
    if (!source_entry) {
        printf("\nInvalid Input");
    }
    char destination[PATH_SIZE];
    char filename[PATH_SIZE] = get_file_name_from_path(source_entry->path_and_name);
    char destinationpath[PATH_SIZE] = the_config->destination;
    strncpy(destination, destinationpath, PATH_SIZE - 1);  
    //have to check on this later !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    destination[PATH_SIZE - 1] = '\0';
     if (destination[strlen(destination) - 1] != '/') {
        // If not, add it
        strncat(destination, "/", PATH_SIZE - strlen(destination) - 1);
        destination[PATH_SIZE - 1] = '\0';  // Ensure null-termination
    }
    // Concatenate the filename to the destination path
    strncat(destination, filename, PATH_SIZE - strlen(destination) - 1);
    destination[PATH_SIZE - 1] = '\0'; 

    //destination file initialization

    files_list_entry_t *destination_entry = (files_list_entry_t *)malloc(sizeof(files_list_entry_t));
    strncpy(destination_entry->path_and_name, destination,PATH_SIZE - 1) ;
    destination[PATH_SIZE - 1] = '\0'; 
    if (get_file_stats(destination_entry, the_config) == -1) {
        printf("\nERROR GETTING FILE STATS");
        free(destination_entry);
        return;
    }

    if (source_entry->entry_type == FICHIER) {
        if (access(destination_entry->path_and_name, F_OK) == -1 || mismatch(source_entry,destination_entry, the_config)) {
            int source_fd = open(source_entry->path_and_name, O_RDONLY);
            int dest_fd = open(destination_entry->path_and_name, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

            if (source_fd == -1 || dest_fd == -1) {
                printf("\nERROR OPENING FILES");
                free(destination_entry);
                return;
            }

            off_t offset = 0;
            ssize_t bytes_copied = sendfile(dest_fd, source_fd, &offset, source_entry->size);

            if (bytes_copied == -1) {
                printf("\nERROR WHEN WRITTING IN THE DESTINATION FILE!");
                close(source_fd);
                close(dest_fd);
                free(destination_entry);
                return;
            }

            struct timespec times[2];
            times[0] = source_entry->mtime;  // atime
            times[1] = source_entry->mtime;  // mtime

            if (utimensat(AT_FDCWD, destination_entry->path_and_name, times, 0) == -1) {    //small problem with the AT_FDCWD!!!!!!!!!!!!!!!!!!!!!!!!!!
                perror("Error setting modification time");
                free(destination_entry);
                return;
            }
        }
    } else if (source_entry->entry_type == DOSSIER) {

        if(access(destination_entry->path_and_name, F_OK) == -1 || mismatch(source_entry, destination_entry, the_config)) {
            if (mkdir(destination_entry->path_and_name, S_IRWXU | S_IRWXG | S_IRWXO) == -1) {
                perror("Error creating directory");
                return;
            }

            struct timespec times[2];
            times[0] = source_entry->mtime;
            times[1] = source_entry->mtime;

            if (utimensat(AT_FDCWD, destination_entry->path_and_name, times, AT_SYMLINK_NOFOLLOW) == -1) {   //normaly the AT_SYMLINK is only to assure ;
                perror("Error setting access modes and mtime for directory");
                free(destination_entry);
                return;
            }

        }
    }
    free(destination_entry);

}

/*!
 * @brief make_list lists files in a location (it recurses in directories)
 * It doesn't get files properties, only a list of paths
 * This function is used by make_files_list and make_files_list_parallel
 * @param list is a pointer to the list that will be built
 * @param target is the target dir whose content must be listed
 */
void make_list(files_list_t *list, char *target) {
     // Verification de la liste afin de voir si elle est vide
    if (!list) {
        fprintf(stderr, "Erreur \n");
        return;
    }

    list->head = NULL;
    list->tail = NULL;

    DIR *dir = open_dir(target);
    if (dir == NULL) {
        perror("Erreur ouverture repertoire");
        return;
    }

    struct dirent *entry;
    while ((entry = get_next_entry(dir)) != NULL) {
        char file_path[PATH_SIZE];
        snprintf(file_path, PATH_SIZE, "%s/%s", target, entry->d_name);
<<<<<<< HEAD
        add_file_entry(list, file_path);
        if (entry->d_type == DT_DIR) {                  //look to what is this one !!!!!!!!
=======

        // Ajoute entree a queue de liste
        if (add_entry_to_tail(list, file_path) == -1) {
            clear_files_list(list);
            closedir(dir);
            return;
        }

        if (entry->d_type == DT_DIR) {
>>>>>>> origin/sync_utility
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
