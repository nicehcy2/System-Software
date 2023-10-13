#include <dirent.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#define KILO 1000
#define MEGA 10000000
#define GIGA 100000000000
#define TERA 1000000000000000

typedef struct cnt {
    size_t dirs;
    size_t files;
} cnt_t;

typedef struct info {
    char *name;
    long inode;
    int device;
    char *mode;
    char *usr_name;
    long size;
    int type;
    struct info *next;
} info_t;

int tree(const char *, cnt_t *, const char *);
char initUnit(info_t *);
void changeSize(info_t *, char);

int main(int argc, char *argv[]) {
    char *dir = argc > 1 ? argv[1] : ".";
    printf("%s\n", dir);

    cnt_t counter = {0, 0};
    tree(dir, &counter, "");

    if (!counter.dirs)
        counter.dirs = 0;
    else
        counter.dirs--;
    printf("\n%zu directories, %zu files\n", counter.dirs, counter.files);
    return 0;
}

int tree(const char *directory, cnt_t *cnt, const char *prefix) {
    struct dirent *dirent;
    struct stat buf;
    struct passwd *pwd;
    DIR *dir_ptr;

    info_t *entry = NULL, *init, *iter;
    size_t size = 0;

    char unit;
    char *path, *seg, *ptr, *next_prefix, *file_name;

    dir_ptr = opendir(directory);
    if (!dir_ptr) {
        perror("opendir() error!");
        return -1;
    }

    cnt->dirs++;

    while ((dirent = readdir(dir_ptr)) != NULL) {
        if (dirent->d_name[0] == '.') {
            continue;
        }

        file_name = malloc(strlen(directory) + strlen(dirent->d_name) + 2);
        sprintf(file_name, "%s/%s", directory, dirent->d_name);

        if (stat(file_name, &buf) == -1) {
            perror("stat() error!");
            return -1;
        }

        pwd = getpwuid(buf.st_uid);
        if (!pwd) {
            perror("getpwuid() error!");
            return -1;
        }

        init = malloc(sizeof(info_t));
        init->name = strcpy(malloc(strlen(dirent->d_name) + 1), dirent->d_name);
        init->inode = dirent->d_ino;
        init->device = buf.st_dev;
        init->usr_name = strcpy(malloc(strlen(pwd->pw_name) + 1), pwd->pw_name);
        init->type = dirent->d_type == DT_DIR;
        init->size = buf.st_size;
        init->mode = malloc(sizeof(char) * 20);
        init->next = NULL;

        if (S_ISDIR(buf.st_mode)) {
            init->mode[0] = 'd';
        } else {
            init->mode[0] = '-';
        }

        if (S_IRUSR & buf.st_mode) {
            init->mode[1] = 'r';
        } else {
            init->mode[1] = '-';
        }

        if (S_IWUSR & buf.st_mode) {
            init->mode[2] = 'w';
        } else {
            init->mode[2] = '-';
        }

        if (S_IXUSR & buf.st_mode) {
            init->mode[3] = 'x';
        } else {
            init->mode[3] = '-';
        }

        if (S_IRGRP & buf.st_mode) {
            init->mode[4] = 'r';
        } else {
            init->mode[4] = '-';
        }

        if (S_IWGRP & buf.st_mode) {
            init->mode[5] = 'w';
        } else {
            init->mode[5] = '-';
        }

        if (S_IXGRP & buf.st_mode) {
            init->mode[6] = 'x';
        } else {
            init->mode[6] = '-';
        }

        if (S_IROTH & buf.st_mode) {
            init->mode[7] = 'r';
        } else {
            init->mode[7] = '-';
        }

        if (S_IWOTH & buf.st_mode) {
            init->mode[8] = 'w';
        } else {
            init->mode[8] = '-';
        }

        if (S_IXOTH & buf.st_mode) {
            init->mode[9] = 'x';
        } else {
            init->mode[9] = '-';
        }

        for (int i = 10; i < 20; i++) {
            init->mode[i] = '\0';
        }

        if (entry == NULL) {
            entry = init;
        } else if (strcmp(init->name, entry->name) < 0) {
            init->next = entry;
            entry = init;
        } else {
            for (iter = entry;
                 iter->next && strcmp(init->name, iter->next->name) > 0;
                 iter = iter->next)
                ;

            init->next = iter->next;
            iter->next = init;
        }

        size++;
        free(file_name);
    }

    closedir(dir_ptr);
    if (!entry) {
        return 0;
    }

    for (size_t idx = 0; idx < size; idx++) {
        if (idx == size - 1) {
            ptr = "└── ";
            seg = "    ";
        } else {
            ptr = "├── ";
            seg = "│   ";
        }

        unit = '\0';
        unit = initUnit(entry);
        changeSize(entry, unit);

        printf("%s%s[ %ld %d %s %s %ld%c ]  %s\n", prefix, ptr, entry->inode,
               entry->device, entry->mode, entry->usr_name, entry->size, unit,
               entry->name);

        if (entry->type) {
            path = malloc(strlen(directory) + strlen(entry->name) + 2);

            sprintf(path, "%s/%s", directory, entry->name);

            next_prefix = malloc(strlen(prefix) + strlen(seg) + 1);

            sprintf(next_prefix, "%s%s", prefix, seg);

            tree(path, cnt, next_prefix);
            free(path);
            free(next_prefix);
        } else {
            cnt->files++;
        }

        init = entry;
        entry = entry->next;

        free(init->name);
        free(init->usr_name);
        free(init->mode);
        free(init);
    }

    return 0;
}

char initUnit(info_t *head) {
    char unit = '\0';
    if (head->size >= KILO && head->size < MEGA)
        unit = 'K';
    else if (head->size >= MEGA && head->size < GIGA)
        unit = 'M';
    else if (head->size >= GIGA && head->size < TERA)
        unit = 'G';
    return unit;
}

void changeSize(info_t *head, char unit) {
    if (unit == 'K') {
        head->size /= 1000;
    } else if (unit == 'M') {
        head->size /= 10000000;
    } else if (unit == 'G') {
        head->size /= 100000000000;
    }
}
