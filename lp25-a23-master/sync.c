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
#include <stdlib.h>
#include <stdio.h>

/*!
 * @brief synchronize is the main function for synchronization
 * It will build the lists (source and destination), then make a third list with differences, and apply differences to the destination
 * It must adapt to the parallel or not operation of the program.
 * @param the_config is a pointer to the configuration
 * @param p_context is a pointer to the processes context
 */
void synchronize(configuration_t *the_config, process_context_t *p_context) {
    //implementation not considering multiprocess!!!!!!!
    files_list_t *destination = (files_list_t *)malloc(sizeof(files_list_t));
    files_list_t *source = (files_list_t *)malloc(sizeof(files_list_t));
    files_list_t *differences = (files_list_t *)malloc(sizeof(files_list_t));
    make_files_list(destination, the_config->destination);
    make_files_list(source, the_config->source);
    
    differences->head = NULL;
    differences->tail = NULL;
    
    files_list_entry_t *source_element = source->head;
    files_list_entry_t *checkelem;

    while (source_element) {
        checkelem = find_entry_by_name(destination, source_element->path_and_name); 
        if (!checkelem) {
            add_entry_to_tail(differences,source_element);
        }
        source_element = source_element->next;
    }
    

    
    if (differences->head) {
        
        files_list_entry_t *diftemp = differences->head;

        while (diftemp) {
            copy_entry_to_destination(diftemp, the_config);
            diftemp = diftemp->next;
        } 
    
    } else {
        printf("\nDifferences list was empty!");
    }

    clear_files_list(differences);
    clear_files_list(destination);
    clear_files_list(source);
   
    
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

    if (!list) {
        perror("\nNULL LIST WAS PROVIDED!");
        return;
    }
    
    files_list_t *path_list = (files_list_t*)sizeof(files_list_t);
    make_list(path_list, target_path);

    files_list_entry_t *temp = path_list->head;

    while (temp) {
        if (add_file_entry(list, temp->path_and_name) == -1) {
            perror("\nERROR WITH add_file_entry");
            clear_files_list(list);
            clear_files_list(path_list);
            return;
        }
        temp = temp->next;
    }
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
void copy_entry_to_destination(files_list_entry_t *source_entry, configuration_t *the_config) {          //theres an error here!!!
    if (!source_entry) {
        printf("\nInvalid Input");
    }
    
    char destination[PATH_SIZE], filename[PATH_SIZE];
    strncpy(destination, the_config->destination, PATH_SIZE - 1);
    destination[PATH_SIZE - 1] = '\0';
    strncpy(filename, get_file_name_from_path(source_entry->path_and_name), PATH_SIZE - 1);
    filename[PATH_SIZE - 1] = '\0';

    //could use snprintf here 
    if (destination[strlen(destination) - 1] != '/') {
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
    
    if (get_file_stats(destination_entry) == -1) {
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
//have to take care when initialising the tial of the list!!!
void make_list(files_list_t *list, char *target) {                             //should use add entry! 
     // Verification de la liste afin de voir si elle est vide
    if (!list) {
        perror("\nNULL LIST WAS PROVIDED!");
        return;
    }

    DIR *dir = open_dir(target);
    if (dir == NULL) {
        perror("Erreur ouverture repertoire");
        return;
    }

    list->head = NULL;
    list->tail = NULL;

    struct dirent *entry;
    while ((entry = get_next_entry(dir)) != NULL) {
       
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        char file_path[PATH_SIZE];
        snprintf(file_path, PATH_SIZE, "%s/%s", target, entry->d_name);

        files_list_entry_t *new_entry = (files_list_entry_t *)malloc(sizeof(files_list_entry_t));
        
        if (!new_entry) {
            perror("Error allocating memory for new_entry");
            closedir(dir);
            return;
        }

        strncpy(new_entry->path_and_name, file_path, PATH_SIZE - 1);
        new_entry->path_and_name[PATH_SIZE - 1] = '\0';

        if (add_entry_to_tail(list, new_entry) == -1) {
            perror("\nERROR IN FUCNTION add_entry_to_tail!");
        }

        /*new_entry->next = NULL;
        new_entry->prev = list->tail;

        if (list->head == NULL) {
            list->head = new_entry;
        }

        if (list->tail != NULL) {
            list->tail->next = new_entry;
        }

        list->tail = new_entry;*/

        if (entry->d_type == DT_DIR) {
            make_list(list, file_path);
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
