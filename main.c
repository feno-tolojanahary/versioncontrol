#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#define WORK_DIR "backup"

void create_dir(char * dir_name) 
{
    struct stat st = {0};
    if (stat(dir_name, &st) == -1) {
        mkdir(dir_name, 0700);
    }
}

int is_directory(const char * path) 
{
    struct stat astatbuf;
    if (stat(path, &astatbuf) != 0) 
        return 0;
    return S_ISDIR(astatbuf.st_mode);
}

void * s_malloc(size_t size) 
{
    void *ptr = malloc(size);
    if (ptr == NULL) {
        printf("Error allocating memory");
        exit(1);
    }
    return ptr;
}

void * s_realloc(void* ptr, size_t size)
{
    ptr = realloc(ptr, size);
    if (ptr == NULL) {
        printf("Error when reallocating memory");
        exit(1);
    }
    return ptr;
}

int copy_content(const char * path, const char * dst_path) 
{
    FILE * subfile; 
    FILE * copyfile;
    int ch;

    subfile = fopen(path, "r");
    copyfile = fopen(dst_path, "w");

    if (copyfile == NULL) {
        printf("error opening copyfile");
        return 1;
    }
    if (subfile == NULL) {
        printf("error opening original file");
        return 1;
    }

    while ((ch = fgetc(subfile)) != EOF) {
        // printf("%c\n", ch);
        fputc(ch, copyfile);
    }
    fclose(subfile);
    fclose(copyfile);
    return 0;    
}

int cpy_file_path (char * filepath, char * relative_path_dr, char * filename) 
{
    char * copyfilepath = s_malloc(sizeof (char) * strlen(relative_path_dr) * strlen(filename) + 2);

    strcpy(copyfilepath, relative_path_dr);
    strcat(copyfilepath, "/");
    strcat(copyfilepath, filename);
    
    copy_content(filepath, copyfilepath);

    free(copyfilepath);
}
 
int snapshot() 
{
    struct dirent *de;
    char path[] = "../sample";
    const char linuxDir[3] = "..";
    const char linuxDir2[2] = ".";
    DIR *dr = opendir(path);

    if (dr == NULL) {
        printf("Could not open directory");
        return 0;
    }

    while((de = readdir(dr)) != NULL) {
        char * subdir_path = s_malloc(sizeof (char) * (strlen(path) + strlen(de->d_name) + 2));
        char * relative_path_subdr = s_malloc(sizeof (char) * strlen(WORK_DIR) + 1);

        strcpy(subdir_path, path);
        strcat(subdir_path, "/");
        strcat(subdir_path, de->d_name);
        
        strcpy(relative_path_subdr, WORK_DIR);
      
        if (strcmp(de->d_name, linuxDir) == 0 || strcmp(de->d_name, linuxDir2) == 0) {
            free(relative_path_subdr);
            free(subdir_path);
            continue;
        }
        if (is_directory(subdir_path) > 0) {
            DIR *subd = opendir(subdir_path);
            struct dirent *subde;
            
            relative_path_subdr = s_realloc(relative_path_subdr, sizeof (char) * (strlen(WORK_DIR) + strlen(de->d_name) + 2));
            strcat(relative_path_subdr, "/");
            strcat(relative_path_subdr, de->d_name);
            
            create_dir(relative_path_subdr);
            
            while ((subde = readdir(subd)) != NULL) {
                char * filepath = s_malloc(sizeof (char) * (strlen(subdir_path) + strlen(subde->d_name) + 2));
                strcpy(filepath, subdir_path);
                
                strcat(filepath, "/");
                strcat(filepath, subde->d_name);

                if (strcmp(subde->d_name, linuxDir) == 0 || strcmp(subde->d_name, linuxDir2) == 0) {
                    free(filepath);
                    continue;
                }
                // read content if a file
                if (is_directory(filepath) == 0) {
                    int res = cpy_file_path(filepath, relative_path_subdr, subde->d_name);
                    if (res == 1) {
                        printf("error copying file");
                    }
                } else {
                    // printf("read directory content");
                }
                free(filepath);
            }
            closedir(subd);
        } else {
            printf("%s is not a directory", subdir_path);

            // if the content is a file make copie of file
            char * org_filepath = s_malloc(sizeof (char) * (strlen(subdir_path) + 1));
            strcpy(org_filepath, subdir_path);
            printf("\ncreate file %s, subdirpath: %s, filename: \"%s\" ", org_filepath, relative_path_subdr, de->d_name);
            int res = cpy_file_path(org_filepath, relative_path_subdr, de->d_name);
            if (res == 1) {
                printf("error copying file");
            }
            free(org_filepath);
        }
        free(relative_path_subdr);
        free(subdir_path);
    //     printf("\n");
    }
    closedir(dr);
    return 0;
}

int main(int argc, char * argv) 
{
    create_dir(WORK_DIR);
    snapshot();
    return 0;
}
