#include "mysh.h"

int setup_redirections(command_t *cmd) {
    int fd;
    
    if (cmd->redir_type == REDIR_NONE) {
        return 0;
    }
    
    switch (cmd->redir_type) {
        case REDIR_IN:
            fd = open(cmd->redir_file, O_RDONLY);
            if (fd < 0) {
                perror(cmd->redir_file);
                return -1;
            }
            dup2(fd, STDIN_FILENO);
            close(fd);
            break;
            
        case REDIR_OUT:
            fd = open(cmd->redir_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) {
                perror(cmd->redir_file);
                return -1;
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
            break;
            
        case REDIR_OUT_APPEND:
            fd = open(cmd->redir_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (fd < 0) {
                perror(cmd->redir_file);
                return -1;
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
            break;
            
        case REDIR_ERR:
            fd = open(cmd->redir_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) {
                perror(cmd->redir_file);
                return -1;
            }
            dup2(fd, STDERR_FILENO);
            close(fd);
            break;
            
        case REDIR_ERR_APPEND:
            fd = open(cmd->redir_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (fd < 0) {
                perror(cmd->redir_file);
                return -1;
            }
            dup2(fd, STDERR_FILENO);
            close(fd);
            break;
            
        case REDIR_BOTH:
            fd = open(cmd->redir_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) {
                perror(cmd->redir_file);
                return -1;
            }
            dup2(fd, STDOUT_FILENO);
            dup2(fd, STDERR_FILENO);
            close(fd);
            break;
            
        case REDIR_BOTH_APPEND:
            fd = open(cmd->redir_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (fd < 0) {
                perror(cmd->redir_file);
                return -1;
            }
            dup2(fd, STDOUT_FILENO);
            dup2(fd, STDERR_FILENO);
            close(fd);
            break;
            
        default:
            break;
    }
    
    return 0;
}

void restore_redirections(int saved_stdin, int saved_stdout, int saved_stderr) {
    if (saved_stdin >= 0) {
        dup2(saved_stdin, STDIN_FILENO);
        close(saved_stdin);
    }
    if (saved_stdout >= 0) {
        dup2(saved_stdout, STDOUT_FILENO);
        close(saved_stdout);
    }
    if (saved_stderr >= 0) {
        dup2(saved_stderr, STDERR_FILENO);
        close(saved_stderr);
    }
}
