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
   
    if (subfile == NULL) {
        printf("error opening original file\n");
        return 1;
    }

    copyfile = fopen(dst_path, "w");

    if (copyfile == NULL) {
        printf("error opening copyfile, dst_path = %s \n", dst_path);
        fclose(subfile);
        return 1;
    }
    while ((ch = fgetc(subfile)) != EOF) {
        // printf("%c\n", ch);
        fputc(ch, copyfile);
    }
    fclose(subfile);
    fclose(copyfile);
    copyfile = NULL;
    return 0;    
}

int cpy_file_path (char * filepath, char * relative_path_dr, char * filename) 
{
    char * copyfilepath = s_malloc(sizeof (char) * strlen(relative_path_dr) * strlen(filename) + 2);

    strcpy(copyfilepath, relative_path_dr);
    strcat(copyfilepath, "/");
    strcat(copyfilepath, filename);
    
    int res = copy_content(filepath, copyfilepath);
    if (res == 1) {
        return 1;
    }
    free(copyfilepath);
    return 0;
}

char * getWkPath() {
    return "../sample";
}

int dupl_dir_content (char * source_path, char * dest_path)
{
    struct dirent *de;
    DIR * dr = opendir(source_path);
    const char linuxDir[3] = "..";
    const char linuxDir2[2] = ".";

    if (dr == NULL) {
        printf("Could not open directory");
        return 1;
    }

    while((de = readdir(dr)) != NULL) {
        char * subdir_path = s_malloc(sizeof (char) * (strlen(source_path) + strlen(de->d_name) + 2));
        strcpy(subdir_path, source_path);
        strcat(subdir_path, "/");
        strcat(subdir_path, de->d_name);

        if (strcmp(de->d_name, linuxDir) == 0 || strcmp(de->d_name, linuxDir2) == 0) {
            free(subdir_path);
            continue;
        }

        if (is_directory(subdir_path) > 0) {
            char * dest_subdir_path = s_malloc(sizeof (char) * (strlen(dest_path) + strlen(de->d_name) + 2));
            strcpy(dest_subdir_path, dest_path);
            strcat(dest_subdir_path, "/");
            strcat(dest_subdir_path, de->d_name);

            // duplicate dir
            create_dir(dest_subdir_path);

            // duplicate dir content
            dupl_dir_content(subdir_path, dest_subdir_path);
            printf("copy folder %s to %s -- OK \n", subdir_path, dest_subdir_path);
            free(dest_subdir_path);
            dest_subdir_path = NULL;
            free(subdir_path);
            subdir_path = NULL;
        } else {
            
            // if the content is a file make copie of file
            char * org_filepath = s_malloc(sizeof (char) * (strlen(subdir_path) + 1));
            strcpy(org_filepath, subdir_path);
            int res = cpy_file_path(org_filepath, dest_path, de->d_name);
            if (res == 1) {
                printf("error copying file");
            } else {
                printf("copy file %s to %s -- OK \n", org_filepath, dest_path);
            }
            free(org_filepath);
            org_filepath = NULL;
            free(subdir_path);
            subdir_path = NULL;
        }
    }
    closedir(dr);
    return 0;
}
 
int snapshot() 
{
    struct dirent *de;
    const char linuxDir[3] = "..";
    const char linuxDir2[2] = ".";

    char * path = getWkPath();
    char * dst_path = WORK_DIR;

    dupl_dir_content(path, dst_path);
    return 0;
}

int main(int argc, char * argv) 
{
    create_dir(WORK_DIR);
    snapshot();
    return 0;
}
