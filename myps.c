#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/stat.h>

#define COLOR_RESET     "\033[0m"
#define COLOR_RUNNING   "\033[1;32m"  /* Vert */
#define COLOR_SLEEPING  "\033[1;34m"  /* Bleu */
#define COLOR_STOPPED   "\033[1;33m"  /* Jaune */
#define COLOR_ZOMBIE    "\033[1;31m"  /* Rouge */
#define COLOR_OTHER     "\033[1;37m"  /* Blanc */

const char *get_state_color(char state) {
    switch (state) {
        case 'R': return COLOR_RUNNING;   /* Running */
        case 'S': return COLOR_SLEEPING;  /* Sleeping */
        case 'D': return COLOR_SLEEPING;  /* Disk sleep */
        case 'T': return COLOR_STOPPED;   /* Stopped */
        case 't': return COLOR_STOPPED;   /* Tracing stop */
        case 'Z': return COLOR_ZOMBIE;    /* Zombie */
        case 'X': return COLOR_ZOMBIE;    /* Dead */
        default:  return COLOR_OTHER;
    }
}

const char *get_state_string(char state) {
    switch (state) {
        case 'R': return "R";
        case 'S': return "S";
        case 'D': return "D";
        case 'T': return "T";
        case 't': return "t";
        case 'Z': return "Z";
        case 'X': return "X";
        default:  return "?";
    }
}

void print_process_info(const char *pid_str) {
    char path[256];
    char line[1024];
    FILE *fp;
    
    /* Lit /proc/[pid]/stat */
    snprintf(path, sizeof(path), "/proc/%s/stat", pid_str);
    fp = fopen(path, "r");
    if (fp == NULL) {
        return;
    }
    
    int pid, ppid, pgrp, session, tty_nr, tpgid;
    unsigned long flags, minflt, cminflt, majflt, cmajflt, utime, stime;
    long cutime, cstime, priority, nice, num_threads, itrealvalue, starttime;
    unsigned long vsize;
    long rss;
    char comm[256], state;
    
    if (fgets(line, sizeof(line), fp) == NULL) {
        fclose(fp);
        return;
    }
    fclose(fp);
    
    /* Parse le fichier stat */
    char *p = strchr(line, '(');
    if (p == NULL) return;
    
    char *q = strrchr(p, ')');
    if (q == NULL) return;
    
    *q = '\0';
    strcpy(comm, p + 1);
    
    sscanf(q + 2, "%c %d %d %d %d %d %lu %lu %lu %lu %lu %lu %lu %ld %ld %ld %ld %ld %ld %lu %lu %ld",
           &state, &ppid, &pgrp, &session, &tty_nr, &tpgid,
           &flags, &minflt, &cminflt, &majflt, &cmajflt, &utime, &stime,
           &cutime, &cstime, &priority, &nice, &num_threads, &itrealvalue, &starttime, &vsize, &rss);
    
    /* Lit /proc/[pid]/status pour l'UID */
    snprintf(path, sizeof(path), "/proc/%s/status", pid_str);
    fp = fopen(path, "r");
    int uid = 0;
    if (fp != NULL) {
        while (fgets(line, sizeof(line), fp) != NULL) {
            if (strncmp(line, "Uid:", 4) == 0) {
                sscanf(line + 4, "%d", &uid);
                break;
            }
        }
        fclose(fp);
    }
    
    /* Récupère le nom d'utilisateur */
    struct passwd *pw = getpwuid(uid);
    char username[32];
    if (pw != NULL) {
        strncpy(username, pw->pw_name, sizeof(username) - 1);
        username[sizeof(username) - 1] = '\0';
    } else {
        snprintf(username, sizeof(username), "%d", uid);
    }
    
    /* Calcule le temps CPU */
    unsigned long total_time = utime + stime;
    unsigned long seconds = total_time / sysconf(_SC_CLK_TCK);
    unsigned long minutes = seconds / 60;
    seconds = seconds % 60;
    
    /* Récupère le TTY */
    char tty[16];
    if (tty_nr == 0) {
        strcpy(tty, "?");
    } else {
        snprintf(tty, sizeof(tty), "pts/%d", tty_nr);
    }
    
    /* Affiche avec couleur selon l'état */
    printf("%-8s %5s %-7s %s%c%s %5ld %02lu:%02lu %s\n",
           username, pid_str, tty,
           get_state_color(state), state, COLOR_RESET,
           priority, minutes, seconds, comm);
}

int is_numeric(const char *str) {
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] < '0' || str[i] > '9') {
            return 0;
        }
    }
    return 1;
}

int main(void) {
    DIR *proc_dir;
    struct dirent *entry;
    
    /* Affiche l'en-tête */
    printf("USER     PID   TTY     STAT   PRI  TIME COMMAND\n");
    
    /* Ouvre le répertoire /proc */
    proc_dir = opendir("/proc");
    if (proc_dir == NULL) {
        perror("opendir /proc");
        return 1;
    }
    
    /* Lit toutes les entrées dans /proc */
    while ((entry = readdir(proc_dir)) != NULL) {
        /* Vérifie si l'entrée est un PID (numérique) */
        if (!is_numeric(entry->d_name)) {
            continue;
        }
        
        print_process_info(entry->d_name);
    }
    
    closedir(proc_dir);
    return 0;
}
