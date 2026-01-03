#include "mysh.h"


void lock_read_env(void) {
    if (shared_env == NULL) return;
    
    sem_wait(&shared_env->mutex);
    if (shared_env->writers_waiting > 0) {
        sem_post(&shared_env->mutex);
        sem_wait(&shared_env->writers_sem);
    } else {
        shared_env->readers_count++;
        if (shared_env->readers_count == 1) {
            sem_wait(&shared_env->readers_sem);
        }
        sem_post(&shared_env->mutex);
    }
}

void unlock_read_env(void) {
    if (shared_env == NULL) return;
    
    sem_wait(&shared_env->mutex);
    shared_env->readers_count--;
    if (shared_env->readers_count == 0) {
        sem_post(&shared_env->readers_sem);
    }
    sem_post(&shared_env->mutex);
}

void lock_write_env(void) {
    if (shared_env == NULL) return;
    
    sem_wait(&shared_env->mutex);
    shared_env->writers_waiting++;
    sem_post(&shared_env->mutex);
    
    sem_wait(&shared_env->readers_sem);
    sem_wait(&shared_env->writers_sem);
    
    sem_wait(&shared_env->mutex);
    shared_env->writers_waiting--;
    sem_post(&shared_env->mutex);
}

void unlock_write_env(void) {
    if (shared_env == NULL) return;
    
    sem_post(&shared_env->writers_sem);
    sem_post(&shared_env->readers_sem);
}

int init_shared_env(char **envp) {
    key_t key = ftok("/tmp", 'M');
    int created = 0;
    
    shmid = shmget(key, SHM_SIZE, 0666);
    
    if (shmid < 0) {
        shmid = shmget(key, SHM_SIZE, IPC_CREAT | 0666);
        if (shmid < 0) {
            perror("shmget");
            return -1;
        }
        created = 1;
    }
    
    shared_env = (shared_env_t *)shmat(shmid, NULL, 0);
    if (shared_env == (void *)-1) {
        perror("shmat");
        return -1;
    }
    
    if (created) {
        shared_env->initialized = 1;
        shared_env->ref_count = 1;
        shared_env->readers_count = 0;
        shared_env->writers_waiting = 0;
        
        sem_init(&shared_env->readers_sem, 1, 1);
        sem_init(&shared_env->writers_sem, 1, 1);
        sem_init(&shared_env->mutex, 1, 1);
        shared_env->data[0] = '\0';
        int offset = 0;
        for (int i = 0; envp[i] != NULL; i++) {
            int len = strlen(envp[i]);
            if (offset + len + 1 < SHM_SIZE - 256) {
                strcpy(shared_env->data + offset, envp[i]);
                offset += len + 1;
            }
        }
        shared_env->data[offset] = '\0';
        shared_env->data[offset + 1] = '\0';
    } else {
        lock_write_env();
        shared_env->ref_count++;
        unlock_write_env();
    }
    
    return 0;
}

void cleanup_shared_env(void) {
    if (shared_env == NULL) {
        return;
    }
    
    lock_write_env();
    shared_env->ref_count--;
    int should_destroy = (shared_env->ref_count == 0);
    unlock_write_env();
    
    if (should_destroy) {
        sem_destroy(&shared_env->readers_sem);
        sem_destroy(&shared_env->writers_sem);
        sem_destroy(&shared_env->mutex);
        
        shmdt(shared_env);
        shmctl(shmid, IPC_RMID, NULL);
    } else {
        shmdt(shared_env);
    }
    
    shared_env = NULL;
}

char *get_variable(char *name) {
    variable_t *var = local_vars;
    while (var != NULL) {
        if (strcmp(var->name, name) == 0) {
            return var->value;
        }
        var = var->next;
    }

    lock_read_env();
    
    int offset = 0;
    while (shared_env->data[offset] != '\0' || shared_env->data[offset + 1] != '\0') {
        char *entry = shared_env->data + offset;
        char *eq = strchr(entry, '=');
        
        if (eq != NULL) {
            int name_len = eq - entry;
            if (strncmp(entry, name, name_len) == 0 && name[name_len] == '\0') {
                static char value[MAX_VAR_VALUE];
                strcpy(value, eq + 1);
                unlock_read_env();
                return value;
            }
        }
        
        offset += strlen(entry) + 1;
    }
    
    unlock_read_env();
    
    return NULL;
}

void set_local_variable(char *name, char *value) {
    variable_t *var = local_vars;
    while (var != NULL) {
        if (strcmp(var->name, name) == 0) {
            free(var->value);
            var->value = strdup(value);
            return;
        }
        var = var->next;
    }
    
    var = malloc(sizeof(variable_t));
    if (var == NULL) {
        perror("malloc");
        return;
    }
    
    var->name = strdup(name);
    var->value = strdup(value);
    var->next = local_vars;
    local_vars = var;
}

void unset_local_variable(char *name) {
    variable_t *prev = NULL;
    variable_t *var = local_vars;
    
    while (var != NULL) {
        if (strcmp(var->name, name) == 0) {
            if (prev == NULL) {
                local_vars = var->next;
            } else {
                prev->next = var->next;
            }
            
            free(var->name);
            free(var->value);
            free(var);
            return;
        }
        
        prev = var;
        var = var->next;
    }
}

void set_env_variable(char *name, char *value) {
    lock_write_env();
    
    int offset = 0;
    int found_offset = -1;
    int old_entry_len = 0;
    
    while (shared_env->data[offset] != '\0' || shared_env->data[offset + 1] != '\0') {
        char *entry = shared_env->data + offset;
        char *eq = strchr(entry, '=');
        
        if (eq != NULL) {
            int name_len = eq - entry;
            if (strncmp(entry, name, name_len) == 0 && name[name_len] == '\0') {
                found_offset = offset;
                old_entry_len = strlen(entry) + 1;
                break;
            }
        }
        
        offset += strlen(entry) + 1;
    }
    
    char new_entry[MAX_VAR_NAME + MAX_VAR_VALUE + 2];
    snprintf(new_entry, sizeof(new_entry), "%s=%s", name, value);
    int new_entry_len = strlen(new_entry) + 1;
    
    if (found_offset >= 0) {
        int remaining = offset - found_offset - old_entry_len;
        memmove(shared_env->data + found_offset + new_entry_len,
                shared_env->data + found_offset + old_entry_len,
                remaining + 2);
        strcpy(shared_env->data + found_offset, new_entry);
    } else {
        strcpy(shared_env->data + offset, new_entry);
        shared_env->data[offset + new_entry_len] = '\0';
    }
    
    unlock_write_env();
}

void unset_env_variable(char *name) {
    lock_write_env();
    
    int offset = 0;
    
    while (shared_env->data[offset] != '\0' || shared_env->data[offset + 1] != '\0') {
        char *entry = shared_env->data + offset;
        char *eq = strchr(entry, '=');
        
        if (eq != NULL) {
            int name_len = eq - entry;
            if (strncmp(entry, name, name_len) == 0 && name[name_len] == '\0') {
                int entry_len = strlen(entry) + 1;
                int remaining_offset = offset + entry_len;
                
                int total_offset = remaining_offset;
                while (shared_env->data[total_offset] != '\0' || 
                       shared_env->data[total_offset + 1] != '\0') {
                    total_offset += strlen(shared_env->data + total_offset) + 1;
                }
                
                memmove(shared_env->data + offset,
                        shared_env->data + remaining_offset,
                        total_offset - remaining_offset + 2);
                
                unlock_write_env();
                return;
            }
        }
        
        offset += strlen(entry) + 1;
    }
    
    unlock_write_env();
}

char *expand_variables(char *str) {
    static char result[MAX_LINE];
    char var_name[MAX_VAR_NAME];
    int i = 0, j = 0;
    
    while (str[i] != '\0') {
        if (str[i] == '$' && (i == 0 || str[i-1] != '\\')) {
            i++;
            int k = 0;
            
            while (str[i] && (isalnum(str[i]) || str[i] == '_')) {
                var_name[k++] = str[i++];
            }
            var_name[k] = '\0';
            
            char *value = get_variable(var_name);
            if (value != NULL) {
                strcpy(result + j, value);
                j += strlen(value);
            }
        } else if (str[i] == '\\' && str[i+1] == '$') {
            result[j++] = '$';
            i += 2;
        } else {
            result[j++] = str[i++];
        }
    }
    
    result[j] = '\0';
    return strdup(result);
}

void print_local_variables(void) {
    variable_t *var = local_vars;
    
    while (var != NULL) {
        printf("%s=%s\n", var->name, var->value);
        var = var->next;
    }
}

void print_env_variables(void) {
    lock_read_env();
    
    int offset = 0;
    while (shared_env->data[offset] != '\0' || shared_env->data[offset + 1] != '\0') {
        printf("%s\n", shared_env->data + offset);
        offset += strlen(shared_env->data + offset) + 1;
    }
    
    unlock_read_env();
}
