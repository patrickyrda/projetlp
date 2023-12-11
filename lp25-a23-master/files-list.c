#include <files-list.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <file-properties.h>
#include <defines.h>

#include <stdio.h>

/*!
 * @brief clear_files_list clears a files list
 * @param list is a pointer to the list to be cleared
 * This function is provided, you don't need to implement nor modify it
 */
void clear_files_list(files_list_t *list) {
    while (list->head) {
        files_list_entry_t *tmp = list->head;
        list->head = tmp->next;
        free(tmp);
    }
}

/*!
 *  @brief add_file_entry adds a new file to the files list.
 *  It adds the file in an ordered manner (strcmp) and fills its properties
 *  by calling stat on the file.
 *  Il the file already exists, it does nothing and returns 0
 *  @param list the list to add the file entry into
 *  @param file_path the full path (from the root of the considered tree) of the file
 *  @return 0 if success, -1 else (out of memory)
 */
int add_file_entry(files_list_t *list, char *file_path) {     //ask about the type of return of the funciton

    files_list_entry_t *exist_check = find_entry_by_name(list, file_path);

    if (exist_check) {
        return 0;
    }

    files_list_entry_t *new_entry = (files_list_entry_t*)malloc(sizeof(files_list_entry_t));

    if (!new_entry) {
        perror("\nFAILED TO ALLOCATE MEMORY FOR NEW_ENTRY");
        return -1;
    }

    //maybe add PATH_SIZE here instead of strlen
    strncpy(new_entry->path_and_name, file_path, strlen(file_path) + 1);
    new_entry->path_and_name[strlen(file_path)] = '\0';

    if (get_file_stats(new_entry) == -1) {
        perror("\nFAILED TO ASSIGN VALUES TO new_entry!");
        return -1;
    }

    new_entry->next = NULL;
    new_entry->prev = NULL;

    if (!list->head && !list->tail) {
        list->head = new_entry;
        list->tail = new_entry;
        return 0;
    }
    files_list_entry_t *current = list->head;

    //the tail will be implemented if it does not exist by make_list
    if (strcmp(new_entry->path_and_name, current->path_and_name) < 0) {
        new_entry->next = list->head;
        list->head->prev = new_entry;
        list->head = new_entry;
    } else {
        while (current->next && strcmp(new_entry->path_and_name, current->next->path_and_name) > 0) {
            current = current->next;
        }
        if (!current->next) {
            current->next = new_entry;
            new_entry->prev = list->tail;
            list->tail = new_entry;
        } else {
            new_entry->next = current->next;
            new_entry->prev = current;
            current->next = new_entry;
            current->next->prev = new_entry;
        }
    }
    //succes
    return 0;
}



/*!
 * @brief add_entry_to_tail adds an entry directly to the tail of the list
 * It supposes that the entries are provided already ordered, e.g. when a lister process sends its list's
 * elements to the main process.
 * @param list is a pointer to the list to which to add the element
 * @param entry is a pointer to the entry to add. The list becomes owner of the entry.
 * @return 0 in case of success, -1 else
 */
int add_entry_to_tail(files_list_t *list, files_list_entry_t *entry) {
    // Vérification paramètres
    if (!list || !entry) {
        return -1;
    }

    files_list_entry_t *new_entry = entry;
    new_entry->prev = NULL;
    new_entry->next = NULL;

    // I am not sure if all those controlls are need since most likely the funciton will be used in list provided by a process , might have to simplify it later 
    if (!list->tail && !list->head) {
        
        list->head = new_entry;
        list->tail = new_entry;
    } else if (!list->tail) {
        list->tail = new_entry;
        list->head->next = new_entry;
        new_entry->prev = list->head; 
    } else if (!list->head) { 
        new_entry->prev = list->tail;
        list->head = list->tail;
        list->tail = new_entry;
    } else { 
        new_entry->prev = list->tail;
        list->tail->next = new_entry;
        list->tail = new_entry;
        new_entry->next = NULL;
    }
    return 0 ;

}
    

/*!
 *  @brief find_entry_by_name looks up for a file in a list
 *  The function uses the ordering of the entries to interrupt its search
 *  @param list the list to look into
 *  @param file_path the full path of the file to look for
 *  @param start_of_src the position of the name of the file in the source directory (removing the source path)
 *  @param start_of_dest the position of the name of the file in the destination dir (removing the dest path)
 *  @return a pointer to the element found, NULL if none were found.
 */
files_list_entry_t *find_entry_by_name(files_list_t *list, char *file_path) {    //MOVED BOTH OF SIZE_T VARIABLES
    
    if (!list || !file_path) {
        return NULL;
    }
    
    files_list_entry_t *current = list->head;
    char *current_name = (char*)malloc(PATH_SIZE);
    char *file_path_name = (char*)malloc(PATH_SIZE);
    if (!current_name || !file_path_name) {
        perror("\nFAILED ALLOCATING MEMORY TO FILE NAME VARIABLES");
        free(current_name);
        free(file_path_name);   
        return NULL;
    }
    strncpy(file_path_name, get_file_name_from_path(file_path), PATH_SIZE);
    file_path_name[PATH_SIZE - 1] = '\0';

    while (current) {
        strncpy(current_name, get_file_name_from_path(current->path_and_name), PATH_SIZE);
        current_name[PATH_SIZE - 1] = '\0';
        
        if (strcmp(current_name, file_path_name) == 0) {
            return current;
        }

        current = current->next;
    }
     
    free(current_name);
    free(file_path_name);
    return NULL;   //the file was not found 
}



/*!
 * @brief display_files_list displays a files list
 * @param list is the pointer to the list to be displayed
 * This function is already provided complete.
 */
void display_files_list(files_list_t *list) {
    if (!list)
        return;
    
    for (files_list_entry_t *cursor=list->head; cursor!=NULL; cursor=cursor->next) {
        printf("%s\n", cursor->path_and_name);
    }
}

/*!
 * @brief display_files_list_reversed displays a files list from the end to the beginning
 * @param list is the pointer to the list to be displayed
 * This function is already provided complete.
 */
void display_files_list_reversed(files_list_t *list) {
    if (!list)
        return;
    
    for (files_list_entry_t *cursor=list->tail; cursor!=NULL; cursor=cursor->prev) {
        printf("%s\n", cursor->path_and_name);
    }
}
