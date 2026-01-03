#include "mysh.h"

job_t *add_job(pid_t pid, char *command) {
    job_t *job = malloc(sizeof(job_t));
    if (job == NULL) {
        perror("malloc");
        return NULL;
    }
    
    job->job_id = ++job_counter;
    job->pid = pid;
    job->command = strdup(command);
    job->state = JOB_RUNNING;
    job->status = 0;
    job->next = job_list;
    job_list = job;
    
    return job;
}

void remove_job(int job_id) {
    job_t *prev = NULL;
    job_t *current = job_list;
    
    while (current != NULL) {
        if (current->job_id == job_id) {
            if (prev == NULL) {
                job_list = current->next;
            } else {
                prev->next = current->next;
            }
            
            free(current->command);
            free(current);
            
            if (job_list == NULL) {
                job_counter = 0;
            }
            
            return;
        }
        prev = current;
        current = current->next;
    }
}

job_t *get_job_by_id(int job_id) {
    job_t *current = job_list;
    
    while (current != NULL) {
        if (current->job_id == job_id) {
            return current;
        }
        current = current->next;
    }
    
    return NULL;
}

job_t *get_job_by_pid(pid_t pid) {
    job_t *current = job_list;
    
    while (current != NULL) {
        if (current->pid == pid) {
            return current;
        }
        current = current->next;
    }
    
    return NULL;
}

void check_background_jobs(void) {
    job_t *current = job_list;
    job_t *next;
    pid_t pid;
    int status;
    
    while (current != NULL) {
        next = current->next;
        
        pid = waitpid(current->pid, &status, WNOHANG | WUNTRACED);
        
        if (pid > 0) {
            if (WIFEXITED(status)) {
                printf("%s (jobs=[%d], pid=%d) terminée avec status=%d\n",
                       current->command, current->job_id, current->pid,
                       WEXITSTATUS(status));
                remove_job(current->job_id);
            } else if (WIFSIGNALED(status)) {
                printf("%s (jobs=[%d], pid=%d) terminée avec status=-1\n",
                       current->command, current->job_id, current->pid);
                remove_job(current->job_id);
            } else if (WIFSTOPPED(status)) {
                current->state = JOB_STOPPED;
            }
        }
        
        current = next;
    }
}

void print_jobs(void) {
    job_t *current = job_list;
    
    while (current != NULL) {
        char *state_str;
        
        switch (current->state) {
            case JOB_RUNNING:
                state_str = "En cours d'exécution";
                break;
            case JOB_STOPPED:
                state_str = "Stoppé";
                break;
            default:
                state_str = "Unknown";
                break;
        }
        
        printf("[%d] %d %s %s\n", current->job_id, current->pid, 
               state_str, current->command);
        
        current = current->next;
    }
}

int get_highest_job_id(void) {
    job_t *current = job_list;
    int highest = -1;
    
    while (current != NULL) {
        if (current->job_id > highest) {
            highest = current->job_id;
        }
        current = current->next;
    }
    
    return highest;
}
