#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>

#define THREADSC 12

typedef struct search_data {
    char *start;
    char *search_for;
    char *path;
} search_data_t;

sem_t threads;
sem_t output;
sem_t data;
size_t matches;
bool done = false;

void die(char *message) {
    fprintf(stderr, "%s: %s\n", message, strerror(errno));
    exit(errno);
}

void *output_loop(void *arg) {
    while (!done) {
        sem_wait(&output);
        size_t m;
        sem_wait(&data);
        m = matches;
        sem_post(&data);
        printf("\rfound %3ld files", m);
        fflush(stdout);
    }
    sem_wait(&output);
    printf("found %3ld files\n", matches);
    return NULL;
}

void *walk(void *sdata) {
    search_data_t *sd = (search_data_t *) sdata;
    sem_wait(&threads);
    DIR* dir = opendir(sd->path);
    if (!dir) die("opendir");
    struct dirent *ent;
    while ((ent = readdir(dir))) {
        if (strcmp(ent->d_name, ".") == 0) continue;
        if (strcmp(ent->d_name, "..") == 0) continue;
        if (strstr(sd->search_for, ent->d_name)) {
            // name matches

        }
        if (ent->d_type == DT_DIR) {
            pthread_t thread;
            char next_path[4096];
            snprintf(next_path, sizeof(next_path), "%s/%s", sd->path, ent->d_name);
            search_data_t next = {
                sd->start,
                sd->search_for,
                next_path
            };
            pthread_create(&thread, NULL, walk, &next);
        }
    }
    closedir(dir);
    return NULL;
}

int main(int argc, char **argv) {
    if (argc < 4) {
        fprintf(stderr, "USAGE: %s <start> <# threads> <file>\n", argv[0]);
        return 1;
    }
    sem_init(&threads, 0, THREADSC);
    sem_init(&output, 0, 1);
    sem_init(&data, 0, 1);
    pthread_t output_thread;
    pthread_create(&output_thread, NULL, output_loop, NULL);
    sem_post(&output);
    done = true;
    sem_post(&output);
    void *res;
    int ret = pthread_join(output_thread, &res);
    if (ret != 0) {
        die("pthread_join");
    }
    return 0;
}

