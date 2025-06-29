#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>

// Enum for diff types
typedef enum {
    COMMON,
    ADD,
    DELETE
} DiffType;

// Struct to hold a diff element
typedef struct {
    DiffType type;
    char content; // Storing single characters for simplicity
    int position;
} Diff;

char* read_file_content(const char* file_path, long* file_size);
Diff* lcs_diff(char *X, char *Y, int m, int n, int* diff_size);
Diff* get_file_diff(const char* source_file_path, const char* existing_file_path, int* diff_size);
void copy_dir_recursive(const char *source_dir, const char *dest_dir);
void apply_diff_to_file(const char* file_path, Diff* diffs, int diff_size);
int copy_file(const char *source_path, const char *dest_path);



int max(int a, int b) {
    return (a > b) ? a : b;
}

// Function to perform LCS diff and return an array of Diff structs
Diff* lcs_diff(char *X, char *Y, int m, int n, int* diff_size) {
    int L[m + 1][n + 1];
    int i, j;

    // Build L[m+1][n+1] in bottom up fashion
    for (i = 0; i <= m; i++) {
        for (j = 0; j <= n; j++) {
            if (i == 0 || j == 0)
                L[i][j] = 0;
            else if (X[i - 1] == Y[j - 1])
                L[i][j] = L[i - 1][j - 1] + 1;
            else
                L[i][j] = max(L[i - 1][j], L[i][j - 1]);
        }
    }

    // Allocate memory for the diff array (max possible size is m + n)
    Diff* diffs = (Diff*)malloc(sizeof(Diff) * (m + n));
    if (diffs == NULL) {
        *diff_size = 0;
        return NULL;
    }
    int diff_count = 0;

    // Backtrack from L[m][n] to build the diff in reverse
    i = m;
    j = n;
    while (i > 0 || j > 0) {
        if (i > 0 && j > 0 && X[i - 1] == Y[j - 1]) {
            diffs[diff_count].type = COMMON;
            diffs[diff_count].content = X[i - 1];
            diffs[diff_count].position = i - 1;
            diff_count++;
            i--;
            j--;
        } else if (j > 0 && (i == 0 || L[i][j - 1] >= L[i - 1][j])) {
            diffs[diff_count].type = ADD;
            diffs[diff_count].content = Y[j - 1];
            diffs[diff_count].position = j - 1;
            diff_count++;
            j--;
        } else if (i > 0 && (j == 0 || L[i][j - 1] < L[i - 1][j])) {
            diffs[diff_count].type = DELETE;
            diffs[diff_count].content = X[i - 1];
            diffs[diff_count].position = i - 1;
            diff_count++;
            i--;
        }
    }

    // Reverse the diff array to get the correct order
    for (i = 0; i < diff_count / 2; i++) {
        Diff temp = diffs[i];
        diffs[i] = diffs[diff_count - 1 - i];
        diffs[diff_count - 1 - i] = temp;
    }

    *diff_size = diff_count;
    return diffs;
}

char* read_file_content(const char* file_path, long* file_size) {
    FILE *file = fopen(file_path, "rb");
    if (file == NULL) {
        perror("Error opening file");
        *file_size = 0;
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    *file_size = ftell(file);
    rewind(file);

    char *buffer = (char*)malloc(*file_size + 1);
    if (buffer == NULL) {
        perror("Error allocating memory for file content");
        fclose(file);
        *file_size = 0;
        return NULL;
    }

    size_t bytes_read = fread(buffer, 1, *file_size, file);
    if (bytes_read != *file_size) {
        perror("Error reading file");
        free(buffer);
        fclose(file);
        *file_size = 0;
        return NULL;
    }

    buffer[*file_size] = '\0';

    fclose(file);
    return buffer;
}

Diff* get_file_diff(const char* source_file_path, const char* existing_file_path, int* diff_size) {
    long source_size = 0;
    long existing_size = 0;

    char* source_content = read_file_content(source_file_path, &source_size);
    if (source_content == NULL) {
        *diff_size = 0;
        return NULL;
    }

    char* existing_content = read_file_content(existing_file_path, &existing_size);
    if (existing_content == NULL) {
        free(source_content);
        *diff_size = 0;
        return NULL;
    }

    Diff* diffs = lcs_diff(source_content, existing_content, source_size, existing_size, diff_size);

    free(source_content);
    free(existing_content);

    return diffs;
}

void copy_dir_recursive(const char *source_dir, const char *dest_dir) {
    struct dirent *entry;
    DIR *dp = opendir(source_dir);

    if (dp == NULL) {
        perror("opendir");
        return;
    }

    mkdir(dest_dir, 0755);

    while ((entry = readdir(dp))) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            char source_path[1024];
            char dest_path[1024];

            snprintf(source_path, sizeof(source_path), "%s/%s", source_dir, entry->d_name);
            snprintf(dest_path, sizeof(dest_path), "%s/%s", dest_dir, entry->d_name);

            struct stat statbuf;
            if (stat(source_path, &statbuf) == 0) {
                if (S_ISDIR(statbuf.st_mode)) {
                    copy_dir_recursive(source_path, dest_path);
                } else {
                    struct stat dest_stat;
                    if (stat(dest_path, &dest_stat) == 0) {
                        printf("Apply with lcs diff\n");
                        int diff_size = 0;
                        Diff* diffs = get_file_diff(source_path, dest_path, &diff_size);
                        if (diffs) {
                            apply_diff_to_file(dest_path, diffs, diff_size);
                            free(diffs);
                        }
                    } else {
                        printf("Normal copy\n");
                        copy_file(source_path, dest_path);
                    }
                }
            } else {
                perror("stat");
            }
        }
    }

    closedir(dp);
}


int copy_file(const char *source_path, const char *dest_path) {
    FILE *source_file, *dest_file;
    char buffer[4096];
    size_t bytes_read;

    // Open the source file for reading
    source_file = fopen(source_path, "rb");
    if (source_file == NULL) {
        perror("Error opening source file");
        return -1;
    }

    // Open the destination file for writing
    dest_file = fopen(dest_path, "wb");
    if (dest_file == NULL) {
        perror("Error opening destination file");
        fclose(source_file);
        return -1;
    }

    // Copy the contents
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), source_file)) > 0) {
        if (fwrite(buffer, 1, bytes_read, dest_file) != bytes_read) {
            perror("Error writing to destination file");
            fclose(source_file);
            fclose(dest_file);
            return -1;
        }
    }

    // Check for read errors
    if (ferror(source_file)) {
        perror("Error reading from source file");
    }

    // Close the files
    fclose(source_file);
    fclose(dest_file);

    return 0; // Success
}

void apply_diff_to_file(const char* file_path, Diff* diffs, int diff_size) {
    // Calculate the size of the new file content.
    int new_size = 0;
    for (int i = 0; i < diff_size; i++) {
        if (diffs[i].type == ADD || diffs[i].type == COMMON) {
            new_size++;
        }
    }

    char* new_content = (char*)malloc(new_size + 1);
    if (new_content == NULL) {
        perror("Error allocating memory for reconstructed content");
        return;
    }

    // Build the new content from the diff.
    int current_pos = 0;
    for (int i = 0; i < diff_size; i++) {
        if (diffs[i].type == ADD || diffs[i].type == COMMON) {
            new_content[current_pos++] = diffs[i].content;
        }
    }
    new_content[current_pos] = '\0';

    // Write the reconstructed content to the file, overwriting it.
    FILE *file = fopen(file_path, "wb");
    if (file == NULL) {
        perror("Error opening file for writing");
        free(new_content);
        return;
    }

    if (fwrite(new_content, 1, new_size, file) != new_size) {
        perror("Error writing reconstructed content to file");
    }

    fclose(file);
    free(new_content);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <source_dir> <dest_dir>\n", argv[0]);
        return 1;
    }

    const char *source_dir = argv[1];
    const char *dest_dir = argv[2];

    printf("Starting copy from %s to %s\n", source_dir, dest_dir);
    copy_dir_recursive(source_dir, dest_dir);
    printf("Copy finished.\n");

    return 0;
}
