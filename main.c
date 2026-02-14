#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

void die(char *message) {
    fprintf(stderr, "%s: %s\n", message, strerror(errno));
    exit(errno);
}

void handle_dir(char *path, char *search_for) {
    pid_t pid = fork();
    if (pid < 0) die("fork");
    if (pid > 0) goto wait;
    // child:
    DIR* dir = opendir(path);
    if (!dir) {
        if (errno != 13) die("opendir");
        exit(errno);
    }
    struct dirent *ent;
    while((ent = readdir(dir)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0) {
            continue;
        }
        if (strcmp(ent->d_name, "..") == 0) {
            continue;
        }
        if (strcmp(ent->d_name, search_for) == 0) {
            printf("%s/%s\n", path, ent->d_name);
        }
        if (ent->d_type == DT_DIR) {
            char next[4096];
            snprintf(next, sizeof(next), "%s/%s", path, ent->d_name);
            handle_dir(next, search_for);
        }
    }
    closedir(dir);
    exit(0);
    // end child
wait:
    int status;
    int res = waitpid(pid, &status, 0);
    if (res < 0) die("waitpid");
}


int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "USAGE: %s <start> <threads> <file>\n", argv[0]);
        return 1;
    }
    char *start_dir = argv[1];
    size_t threads = strtol(argv[2], NULL, 10);
    char *search_for = argv[3];
    printf("%d threads\n", threads);
    handle_dir(start_dir, search_for);

    return 0;
}

