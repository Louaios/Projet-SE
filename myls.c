#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <errno.h>

#define COLOR_RESET   "\033[0m"
#define COLOR_DIR     "\033[1;34m"
#define COLOR_EXEC    "\033[1;32m"
#define COLOR_LINK    "\033[1;36m"
#define COLOR_FIFO    "\033[1;33m"
#define COLOR_SOCK    "\033[1;35m"
#define COLOR_CHAR    "\033[1;33m"
#define COLOR_BLOCK   "\033[1;33m"

int show_hidden = 0;
int recursive = 0;

const char *get_color(mode_t mode) {
    if (S_ISDIR(mode)) return COLOR_DIR;
    if (S_ISLNK(mode)) return COLOR_LINK;
    if (S_ISFIFO(mode)) return COLOR_FIFO;
    if (S_ISSOCK(mode)) return COLOR_SOCK;
    if (S_ISCHR(mode)) return COLOR_CHAR;
    if (S_ISBLK(mode)) return COLOR_BLOCK;
    if (mode & S_IXUSR) return COLOR_EXEC;
    return COLOR_RESET;
}

char get_file_type(mode_t mode) {
    if (S_ISDIR(mode)) return 'd';
    if (S_ISLNK(mode)) return 'l';
    if (S_ISFIFO(mode)) return 'p';
    if (S_ISSOCK(mode)) return 's';
    if (S_ISCHR(mode)) return 'c';
    if (S_ISBLK(mode)) return 'b';
    return '-';
}

void print_permissions(mode_t mode) {
    printf("%c", get_file_type(mode));
    printf("%c", (mode & S_IRUSR) ? 'r' : '-');
    printf("%c", (mode & S_IWUSR) ? 'w' : '-');
    printf("%c", (mode & S_IXUSR) ? 'x' : '-');
    printf("%c", (mode & S_IRGRP) ? 'r' : '-');
    printf("%c", (mode & S_IWGRP) ? 'w' : '-');
    printf("%c", (mode & S_IXGRP) ? 'x' : '-');
    printf("%c", (mode & S_IROTH) ? 'r' : '-');
    printf("%c", (mode & S_IWOTH) ? 'w' : '-');
    printf("%c", (mode & S_IXOTH) ? 'x' : '-');
}

void print_file_info(const char *path, const char *name) {
    struct stat st;
    char fullpath[4096];
    
    snprintf(fullpath, sizeof(fullpath), "%s/%s", path, name);
    
    if (lstat(fullpath, &st) < 0) {
        perror(fullpath);
        return;
    }
    
    print_permissions(st.st_mode);
    
    printf(" %3ld", (long)st.st_nlink);
    
    struct passwd *pw = getpwuid(st.st_uid);
    struct group *gr = getgrgid(st.st_gid);
    
    if (pw) printf(" %-8s", pw->pw_name);
    else printf(" %-8d", st.st_uid);
    
    if (gr) printf(" %-8s", gr->gr_name);
    else printf(" %-8d", st.st_gid);
    
    printf(" %8ld", (long)st.st_size);
    
    char timebuf[80];
    struct tm *tm = localtime(&st.st_mtime);
    strftime(timebuf, sizeof(timebuf), "%b %d %H:%M", tm);
    printf(" %s", timebuf);
    
    printf(" %s%s%s", get_color(st.st_mode), name, COLOR_RESET);
    
    if (S_ISLNK(st.st_mode)) {
        char link[1024];
        ssize_t len = readlink(fullpath, link, sizeof(link) - 1);
        if (len >= 0) {
            link[len] = '\0';
            printf(" -> %s", link);
        }
    }
    
    printf("\n");
}

int compare_names(const void *a, const void *b) {
    return strcmp(*(const char **)a, *(const char **)b);
}

void list_directory(const char *path, int depth) {
    DIR *dir;
    struct dirent *entry;
    char **entries = NULL;
    int count = 0;
    int capacity = 100;
    
    if (depth > 0) {
        printf("\n%s:\n", path);
    }
    
    dir = opendir(path);
    if (dir == NULL) {
        perror(path);
        return;
    }
    
    entries = malloc(capacity * sizeof(char *));
    if (entries == NULL) {
        perror("malloc");
        closedir(dir);
        return;
    }
    
    while ((entry = readdir(dir)) != NULL) {
        if (!show_hidden && entry->d_name[0] == '.') {
            continue;
        }
        
        if (count >= capacity) {
            capacity *= 2;
            entries = realloc(entries, capacity * sizeof(char *));
            if (entries == NULL) {
                perror("realloc");
                closedir(dir);
                return;
            }
        }
        
        entries[count++] = strdup(entry->d_name);
    }
    
    closedir(dir);
    
    qsort(entries, count, sizeof(char *), compare_names);
    
    for (int i = 0; i < count; i++) {
        print_file_info(path, entries[i]);
    }
    
    if (recursive) {
        for (int i = 0; i < count; i++) {
            struct stat st;
            char fullpath[4096];
            
            if (strcmp(entries[i], ".") == 0 || strcmp(entries[i], "..") == 0) {
                continue;
            }
            
            snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entries[i]);
            
            if (lstat(fullpath, &st) == 0 && S_ISDIR(st.st_mode)) {
                list_directory(fullpath, depth + 1);
            }
        }
    }
    
    for (int i = 0; i < count; i++) {
        free(entries[i]);
    }
    free(entries);
}

int main(int argc, char *argv[]) {
    int i;
    int has_paths = 0;
    char **paths = malloc(argc * sizeof(char *));
    int path_count = 0;
    
    for (i = 1; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] != '\0') {
            for (int j = 1; argv[i][j] != '\0'; j++) {
                switch (argv[i][j]) {
                    case 'a':
                        show_hidden = 1;
                        break;
                    case 'R':
                        recursive = 1;
                        break;
                    default:
                        fprintf(stderr, "myls: invalid option -- '%c'\n", argv[i][j]);
                        free(paths);
                        return 1;
                }
            }
        } else {
            paths[path_count++] = argv[i];
            has_paths = 1;
        }
    }
    
    if (!has_paths) {
        paths[path_count++] = ".";
    }
    
    for (i = 0; i < path_count; i++) {
        if (path_count > 1 && i > 0) {
            printf("\n");
        }
        
        struct stat st;
        if (lstat(paths[i], &st) < 0) {
            perror(paths[i]);
            continue;
        }
        
        if (S_ISDIR(st.st_mode)) {
            list_directory(paths[i], path_count > 1 ? 1 : 0);
        } else {
            print_file_info(".", paths[i]);
        }
    }
    
    free(paths);
    return 0;
}
