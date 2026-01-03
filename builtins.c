#include "mysh.h"

int is_builtin(char *cmd) {
    return (strcmp(cmd, "cd") == 0 ||
            strcmp(cmd, "exit") == 0 ||
            strcmp(cmd, "status") == 0 ||
            strcmp(cmd, "myjobs") == 0 ||
            strcmp(cmd, "myfg") == 0 ||
            strcmp(cmd, "mybg") == 0 ||
            strcmp(cmd, "set") == 0 ||
            strcmp(cmd, "unset") == 0 ||
            strcmp(cmd, "setenv") == 0 ||
            strcmp(cmd, "unsetenv") == 0);
}

int execute_builtin(command_t *cmd) {
    if (strcmp(cmd->argv[0], "cd") == 0) {
        return builtin_cd(cmd->argv);
    } else if (strcmp(cmd->argv[0], "exit") == 0) {
        return builtin_exit(cmd->argv);
    } else if (strcmp(cmd->argv[0], "status") == 0) {
        return builtin_status(cmd->argv);
    } else if (strcmp(cmd->argv[0], "myjobs") == 0) {
        return builtin_myjobs(cmd->argv);
    } else if (strcmp(cmd->argv[0], "myfg") == 0) {
        return builtin_myfg(cmd->argv);
    } else if (strcmp(cmd->argv[0], "mybg") == 0) {
        return builtin_mybg(cmd->argv);
    } else if (strcmp(cmd->argv[0], "set") == 0) {
        if (cmd->argc == 1) {
            print_local_variables();
            return 0;
        }
        char *eq = strchr(cmd->argv[1], '=');
        if (eq == NULL) {
            fprintf(stderr, "set: invalid format, use var=value\n");
            return 1;
        }
        *eq = '\0';
        set_local_variable(cmd->argv[1], eq + 1);
        *eq = '=';
        return 0;
    } else if (strcmp(cmd->argv[0], "unset") == 0) {
        if (cmd->argc < 2) {
            fprintf(stderr, "unset: missing argument\n");
            return 1;
        }
        unset_local_variable(cmd->argv[1]);
        return 0;
    } else if (strcmp(cmd->argv[0], "setenv") == 0) {
        if (cmd->argc == 1) {
            print_env_variables();
            return 0;
        }
        char *eq = strchr(cmd->argv[1], '=');
        if (eq == NULL) {
            fprintf(stderr, "setenv: invalid format, use var=value\n");
            return 1;
        }
        *eq = '\0';
        set_env_variable(cmd->argv[1], eq + 1);
        *eq = '=';
        return 0;
    } else if (strcmp(cmd->argv[0], "unsetenv") == 0) {
        if (cmd->argc < 2) {
            fprintf(stderr, "unsetenv: missing argument\n");
            return 1;
        }
        unset_env_variable(cmd->argv[1]);
        return 0;
    }
    
    return 1;
}

int builtin_cd(char **argv) {
    char *path;
    
    if (argv[1] == NULL) {
        path = getenv("HOME");
        if (path == NULL) {
            fprintf(stderr, "cd: HOME not set\n");
            return 1;
        }
    } else {
        path = expand_tilde(argv[1]);
    }
    
    if (chdir(path) < 0) {
        perror("cd");
        return 1;
    }
    
    return 0;
}

int builtin_exit(char **argv) {
    (void)argv;
    
    /* exit */
    cleanup_shared_env();
    exit(0);
}

int builtin_status(char **argv) {
    (void)argv;
    
    if (last_command == NULL) {
        printf("No command executed yet\n");
        return 0;
    }
    
    if (last_status == -1) {
        printf("%s terminé anormalement\n", last_command);
    } else {
        printf("%s terminé avec comme code de retour %d\n", last_command, last_status);
    }
    
    return 0;
}

int builtin_myjobs(char **argv) {
    (void)argv;
    print_jobs();
    return 0;
}

int builtin_myfg(char **argv) {
    int job_id;
    job_t *job;
    int status;
    
    if (argv[1] != NULL) {
        job_id = atoi(argv[1]);
        job = get_job_by_id(job_id);
    } else {
        job_id = get_highest_job_id();
        job = get_job_by_id(job_id);
    }
    
    if (job == NULL) {
        fprintf(stderr, "myfg: no such job\n");
        return 1;
    }
    
    /* SIGCONT  */
    if (job->state == JOB_STOPPED) {
        kill(job->pid, SIGCONT);
    }
    
    foreground_pid = job->pid;
    tcsetpgrp(STDIN_FILENO, job->pid);
    
    /* Wait */
    if (waitpid(job->pid, &status, WUNTRACED) < 0) {
        perror("waitpid");
        return 1;
    }
    
    tcsetpgrp(STDIN_FILENO, getpgrp());
    foreground_pid = -1;
    
    if (WIFEXITED(status) || WIFSIGNALED(status)) {
        remove_job(job_id);
    } else if (WIFSTOPPED(status)) {
        job->state = JOB_STOPPED;
        printf("\n[%d] %d Stoppé %s\n", job_id, job->pid, job->command);
    }
    
    return 0;
}

int builtin_mybg(char **argv) {
    int job_id;
    job_t *job;
    
    if (argv[1] != NULL) {
        job_id = atoi(argv[1]);
        job = get_job_by_id(job_id);
    } else {
        job_id = get_highest_job_id();
        job = get_job_by_id(job_id);
    }
    
    if (job == NULL) {
        fprintf(stderr, "mybg: no such job\n");
        return 1;
    }
    
    if (job->state == JOB_RUNNING) {
        fprintf(stderr, "mybg: job already in background\n");
        return 1;
    }
    
    kill(job->pid, SIGCONT);
    job->state = JOB_RUNNING;
    
    printf("[%d] %d %s &\n", job_id, job->pid, job->command);
    
    return 0;
}
