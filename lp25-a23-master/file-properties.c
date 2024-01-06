#include <file-properties.h>
#include <openssl/md5.h>
#include <sys/stat.h>
#include <dirent.h>
#include <openssl/evp.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <defines.h>
#include <fcntl.h>
#include <stdio.h>
#include <utility.h>
#include <ctype.h>


/*!
 * @brief get_file_stats gets all of the required information for a file (inc. directories)
 * @param the files list entry
 * You must get:
 * - for files:
 *   - mode (permissions)
 *   - mtime (in nanoseconds)
 *   - size
 *   - entry type (FICHIER)
 *   - MD5 sum
 * - for directories:
 *   - mode
 *   - entry type (DOSSIER)
 * @return -1 in case of error, 0 else
 */
int get_file_stats(files_list_entry_t *entry) {
    struct stat statbuf;

    if (stat(entry->path_and_name, &statbuf) == -1) {       
        return -1;
    }

    entry->mode = statbuf.st_mode;
    entry->mtime.tv_sec = statbuf.st_mtime;


    if (S_ISREG(statbuf.st_mode)) {
        entry->size = statbuf.st_size;
        entry->entry_type = FICHIER;                     
      
        if (compute_file_md5(entry) == -1) {
            return -1;
        }
        
    } else if (S_ISDIR(statbuf.st_mode)) {
        entry->entry_type = DOSSIER;
    }

    return 0;
}

/*!
 * @brief compute_file_md5 computes a file's MD5 sum
 * @param the pointer to the files list entry
 * @return -1 in case of error, 0 else
 * Use libcrypto functions from openssl/evp.h
 */
int compute_file_md5(files_list_entry_t *entry) {
    FILE *file = fopen(entry->path_and_name, "rb");
    if (!file) {
        return -1;
    }

    unsigned char c[MD5_DIGEST_LENGTH];
    EVP_MD_CTX *mdContext;
    const EVP_MD *md = EVP_md5(); // Use MD5 algorithm
    int bytes;
    unsigned char data[1024];

    mdContext = EVP_MD_CTX_new();
    EVP_DigestInit_ex(mdContext, md, NULL);

    while ((bytes = fread(data, 1, 1024, file)) != 0) {
        EVP_DigestUpdate(mdContext, data, bytes);
    }

    EVP_DigestFinal_ex(mdContext, c, NULL);
    EVP_MD_CTX_free(mdContext);

    for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
        // Convert two hexadecimal characters to one byte
        sscanf((char*)&c[i * 2], "%2hhx", &entry->md5sum[i]);
    }
    entry->md5sum[MD5_DIGEST_LENGTH] = '\0';  // Add null terminator

    fclose(file);
    return 0;
}

/*!
 * @brief directory_exists tests the existence of a directory
 * @path_to_dir a string with the path to the directory
 * @return true if directory exists, false else
 */
bool directory_exists(char *path_to_dir) {
    struct stat statbuf;
    return (stat(path_to_dir, &statbuf) == 0 && S_ISDIR(statbuf.st_mode));
}

/*!
 * @brief is_directory_writable tests if a directory is writable
 * @param path_to_dir the path to the directory to test
 * @return true if dir is writable, false else
 * Hint: try to open a file in write mode in the target directory.
 */
bool is_directory_writable(char *path_to_dir) {
    if (access(path_to_dir, W_OK) == -1) {
        return false;
    }
    return true;
}
//make error control of this function later!!!!!!

char *get_file_name_from_path(char *path) {
    
    if (!path) {
        return NULL;
    }

    char *last_separator = strrchr(path, '/');
    return (last_separator != NULL) ? (last_separator + 1) : path;
}

char *get_path_from_full_path(char *path, char *source_path) {               //have to handle the case when is an absolute path
    
    if (!path || !source_path) {
        return NULL;
    }

    
    if (!path || !source_path) {
        return NULL;
    }

    int count = 0;

    while (path[count] != '\0' && source_path[count] != '\0' && path[count] == source_path[count]) {
        count++;
    }

    char *result = (char *)malloc(PATH_SIZE * sizeof(char));

    if (!result) {
        return NULL;
    }

    strncpy(result, path + count, PATH_SIZE);

    return result;
}





