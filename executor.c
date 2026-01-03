#include "mysh.h"

int execute_simple_command(command_t *cmd) {
    pid_t pid;
    int status;
    
    if (cmd->argc == 0) {
        return 0;
    }
    
    /* Check builtin */
    if (is_builtin(cmd->argv[0])) {
        return execute_builtin(cmd);
    }
    
    /* Fork and execute */
    pid = fork();
    if (pid < 0) {
        perror("fork");
        return -1;
    }
    
    if (pid == 0) {
        /* processus fils */
        
        if (setup_redirections(cmd) < 0) {
            exit(1);
        }
        
        execvp(cmd->argv[0], cmd->argv);
        perror(cmd->argv[0]);
        exit(127);
    }
    
    /* processus parent */
    foreground_pid = pid;
    
    if (last_command != NULL) {
        free(last_command);
    }
    last_command = strdup(cmd->argv[0]);
    
    if (waitpid(pid, &status, WUNTRACED) < 0) {
        perror("waitpid");
        return -1;
    }
    
    foreground_pid = -1;
    
    if (WIFEXITED(status)) {
        last_status = WEXITSTATUS(status);
        return last_status;
    } else if (WIFSIGNALED(status)) {
        last_status = -1;
        return -1;
    } else if (WIFSTOPPED(status)) {
        printf("\n[%d] %d Stoppé %s\n", ++job_counter, pid, cmd->argv[0]);
        job_t *job = add_job(pid, cmd->argv[0]);
        if (job != NULL) {
            job->state = JOB_STOPPED;
        }
        return 0;
    }
    
    return 0;
}

int execute_pipeline(command_t *cmd) {
    int num_cmds = 0;
    int pipefd[MAX_ARGS][2];
    pid_t pids[MAX_ARGS];
    command_t *current;
    int i = 0;
    int status;
    
    //nb commandes
    for (current = cmd; current != NULL; current = current->next) {
        num_cmds++;
    }
    
    for (i = 0; i < num_cmds - 1; i++) {
        if (pipe(pipefd[i]) < 0) {
            perror("pipe");
            return -1;
        }
    }
    
    i = 0;
    for (current = cmd; current != NULL; current = current->next) {
        pids[i] = fork();
        
        if (pids[i] < 0) {
            perror("fork");
            return -1;
        }
        
        if (pids[i] == 0) {
            
            if (i > 0) {
                dup2(pipefd[i-1][0], STDIN_FILENO);
            }
            
            if (i < num_cmds - 1) {
                dup2(pipefd[i][1], STDOUT_FILENO);
            }
            
            for (int j = 0; j < num_cmds - 1; j++) {
                close(pipefd[j][0]);
                close(pipefd[j][1]);
            }
            
            if (setup_redirections(current) < 0) {
                exit(1);
            }
            
            if (is_builtin(current->argv[0])) {
                exit(execute_builtin(current));
            }
            
            execvp(current->argv[0], current->argv);
            perror(current->argv[0]);
            exit(127);
        }
        
        i++;
    }
    
    for (i = 0; i < num_cmds - 1; i++) {
        close(pipefd[i][0]);
        close(pipefd[i][1]);
    }
    
    for (i = 0; i < num_cmds; i++) {
        if (waitpid(pids[i], &status, 0) < 0) {
            perror("waitpid");
        }
    }
    
    if (WIFEXITED(status)) {
        last_status = WEXITSTATUS(status);
        return last_status;
    }
    
    return 0;
}

int execute_command(command_t *cmd, cmd_type_t type, int background) {
    pid_t pid;
    int status;
    
    if (cmd == NULL || cmd->argc == 0) {
        return 0;
    }
    
    if (background) {
        pid = fork();
        if (pid < 0) {
            perror("fork");
            return -1;
        }
        
        if (pid == 0) {
            setpgid(0, 0); 
            
            if (type == CMD_PIPE) {
                execute_pipeline(cmd);
            } else {
                execute_simple_command(cmd);
            }
            
            exit(last_status);
        }
        
        job_t *job = add_job(pid, cmd->argv[0]);
        if (job != NULL) {
            printf("[%d] %d\n", job->job_id, pid);
        }
        
        return 0;
    }
    
    switch (type) {
        case CMD_SIMPLE:
            return execute_simple_command(cmd);
            
        case CMD_PIPE:
            return execute_pipeline(cmd);
            
        case CMD_SEQUENCE:
            status = execute_simple_command(cmd);
            if (cmd->next != NULL) {
                return execute_command(cmd->next, CMD_SIMPLE, 0);
            }
            return status;
            
        case CMD_AND:
            status = execute_simple_command(cmd);
            if (status == 0 && cmd->next != NULL) {
                return execute_command(cmd->next, CMD_SIMPLE, 0);
            }
            return status;
            
        case CMD_OR:
            status = execute_simple_command(cmd);
            if (status != 0 && cmd->next != NULL) {
                return execute_command(cmd->next, CMD_SIMPLE, 0);
            }
            return status;
            
        default:
            return 0;
    }
}
