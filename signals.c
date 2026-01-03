#include "mysh.h"

//Hnadlers de signaux
void setup_signals(void) {
    struct sigaction sa;
    
    /* SIGCHLD handler */
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa, NULL);
    
    /* SIGINT handler */
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa, NULL);
    
    /* SIGTSTP handler */
    sa.sa_handler = sigtstp_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGTSTP, &sa, NULL);
}

void sigchld_handler(int sig) {
    (void)sig;
    int saved_errno = errno;
    pid_t pid;
    int status;
    
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        job_t *job = get_job_by_pid(pid);
        if (job != NULL) {
            if (WIFEXITED(status)) {
                job->status = WEXITSTATUS(status);
                job->state = JOB_DONE;
            } else if (WIFSIGNALED(status)) {
                job->status = -1;
                job->state = JOB_DONE;
            }
        }
    }
    
    errno = saved_errno;
}

void sigint_handler(int sig) {
    (void)sig;
    
    /* If foreground process exists, send signal to it */
    if (foreground_pid > 0) {
        kill(foreground_pid, SIGINT);
        return;
    }
    
    /* Otherwise, ask for confirmation to exit */
    printf("\nVoulez-vous vraiment quitter? (o/n) ");
    fflush(stdout);
    
    char response;
    if (read(STDIN_FILENO, &response, 1) > 0) {
        if (response == 'o' || response == 'O') {
            /* Kill all background jobs */
            job_t *current = job_list;
            while (current != NULL) {
                kill(current->pid, SIGKILL);
                current = current->next;
            }
            
            cleanup_shared_env();
            exit(0);
        }
    }
    
    printf("\n");
    print_prompt();
}

void sigtstp_handler(int sig) {
    (void)sig;
    
    /* If foreground process exists, send signal to it */
    if (foreground_pid > 0) {
        kill(foreground_pid, SIGTSTP);
    }
}
