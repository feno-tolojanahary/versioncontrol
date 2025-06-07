#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#define WORK_DIR "backup"

int max(int a, int b);
void * s_malloc(size_t size);
void * s_realloc(void* ptr, size_t size);

enum Action { INSERT, DELETE };

struct Update {
    Action type;
    char ch;
    int pos;
}

struct UpdateList {
    Update* arr;
    size_t size;
    size_t max_cap;
}

void initialize_arr (UpdateList* upd_list) 
{
    upd_list->arr = (Update*)s_malloc(sizeof(Update) * 50);
    upd_list->size = 0;
    upd_list->max_cap = 50;
}

void add_update_list (UpdateList* upd_list, Update* upd) 
{
    size_t size = upd_list->size;
    if (size == upd_list->max_cap) {
        upd_list->arr = (Update*)s_realloc(upd_list->arr, sizeof(Update) * size * 2);
        upd_list->max_cap = size * 2;
    }
    size++;
    upd_list->arr[size].type = upd->type;
    upd_list->arr[size].ch = upd->ch;
    upd_list->arr[size].pos = upd->pos;
}

void free_upd_list(UpdateList* upd_list) 
{
    upd_list->arr = NULL;
    upd_list->size = 0;
    upd_list->max_cap = 0;
}

void create_dir(char * dir_name) 
{
    struct stat st = {0};
    if (stat(dir_name, &st) == -1) {
        mkdir(dir_name, 0700);
    }
}

int is_directory(const char * path) ee
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

char* lcs(const char* s1, const char* s2)
{
    int m = strlen(s1); 
    int n = strlen(s2);

    int dp[m+1][n+1];

    for (int i = 0; i <= m; i++) {
        for (int j = 0; j <= n; j++) {
            if (i == 0 || j == 0) {
                dp[i][j] = 0;
            } else if (s1[i-1] == s2[j-1]) {
                dp[i][j] = dp[i-1][j-1] + 1;
            } else { 
                dp[i][j] = max(dp[i-1][j], dp[i][j-1]);
            }
        }
    }

    int index = dp[m][n];
    
    char * lcsStr = (char *)malloc((index + 1) * sizeof(char))  ;
    lcsStr[index] = '\0';

    int i = m, j = n;
    while (i > 0 && j > 0) {
        if (s1[i-1] == s2[j-1]) {
            lcsStr[--index] = s1[i-1];
            i--;
            j--;
        } else if (dp[i - 1][j] > dp[i][j - 1]) {
            i--;
        } else {
            j--;
        }
    }
    return lcsStr;
}

UpdateList* generateUpdateScript(char* orig, char* mod, char* lcs, UpdateList* upd_list) {   
    int i = 0, j = 0, k = 0;
    int pos = 0;

    Update* upd = s_malloc(sizeof(Update) * sizeof(lcs));
    
    while(orig[i] || mod[j]) {
        if (lcs[k] && orig[i] == lcs[k] && mod[j] == lcs[k]) {
            i++; j++; pos++;
        } else if (lcs[k] && orig[i] == lcs[k]) {
            // character was added on mod
            upd->type = INSERT;
            upd->ch = orig[i];
            upd->pos = pos;
            add_update_list(upd_list, upd);
            j++; pos++;
        } else if (lcs[k] && mod[j] == lcs[k]) {
            // character was removed on orig
            upd->type = DELETE;
            upd->ch = mod[j];
            upd->pos = pos;
            add_update_list(upd_list, upd);
            i++;
        } else {
            // character deleted and added
            if (orig[i]) {
                // deleted from orig
                upd->type = DELETE;
                upd->ch = orig[i];
                upd->pos = pos;
                add_update_list(upd_list, upd);
                i++;
            } 
            if (mod[k]) {
                // add to mod
                upd->type = INSERT;
                upd->ch = mod[k];
                upd->pos = pos;
                add_update_list(upd_list, upd);
                j++; pos++;
            }
        }
    }
    free(upd);
}

char* applyUpdateScript(UpdateList* updateLst, char* original) 
{
    size_t max = updateLst->size + strlen(original);
    char* result = s_malloc(sizeof(char) * max);
    strcpy(result, original);
    size_t len = strlen(result);

    for (int e = 0; e < updateLst->size; e++) {
        Update* upd = updateLst[e];
        if (upd->type == INSERT) {
            // move string to the rigth
            for (int i = len; i > upd->pos; i--) {
                result[i] = result[i-1];
            }
            len++;
            result[upd->pos] = upd->ch;
            result[len] = '\0';
        } else if (upd->type == DELETE) {
            // move string to the left
            for (int i = upd->pos; i < len; i++) {
                result[i] = result[i+1];
            }
            len--;
            result[len] = '\0';
        }
    }
    return result;
}

int max(int a, int b) {
    if (a > b) {
        return a;
    }
    return b;
}

// void compare_file (char * newfile_path, char * extfile_path)
// {
//     FILE * filenew;
//     FILE * existfile;
//     long int topsize = 0;
//     int ch;
//     filenew = fopen(newfile_path, "r");
//     existfile = fopen(extfile_path, "r");

//     if (filenew == NULL) {
//         printf("error opening new file \n");
//         return;
//     }
    
//     if (existfile == NULL) {
//         printf("error opening existing file \n");
//         return;
//     }

//     fseek(filenew, 0L, SEEK_END);
//     long int fnew_size = ftell(filenew);
//     rewind(filenew);
//     fseek(existfile, 0L, SEEK_END);
//     long int fexist_size = ftell(existfile);
//     rewind(existfile);

//     topsize = fnew_size > fexist_size ? fnew_size : fexist_size;

//     return topsize;
// }

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
    // create_dir(WORK_DIR);
    // snapshot();

    char str1[] = "AGGTAB";
    char str2[] = "GXTXAYB";
    char * result = lcs(str1, str2);
    printf("result lcs: \"%s\" \n", result);  
    free(result);

    return 0;
}
