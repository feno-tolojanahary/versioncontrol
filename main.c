#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

void init_wk() 
{
    struct stat st = {0};
    char * dir = "git";
    if (stat(dir, &st) == -1) {
        mkdir(dir, 0700);
    }
}

 
int snapshot() 
{
    struct dirent *de;
    DIR *dr = opendir("../sample");

    if (dr == NULL) {
        printf("Could not open directory");
        return 0;
    }

    while((de = readdir(dr)) != NULL) {
        printf("%s\n", de->d_name);
    }
    closedir(dr);
    return 0;
}

int main(int argc, char * argv) 
{
    init_wk();
    snapshot();
    return 0;
}
