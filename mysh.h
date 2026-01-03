#ifndef MYSH_H
#define MYSH_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <glob.h>
#include <dirent.h>
#include <pwd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>

#define MAX_LINE 4096
#define MAX_ARGS 256
#define MAX_JOBS 100
#define MAX_VAR_NAME 256
#define MAX_VAR_VALUE 4096
#define SHM_SIZE 65536

/* Command types */
typedef enum {
    CMD_SIMPLE,
    CMD_PIPE,
    CMD_SEQUENCE,
    CMD_AND,
    CMD_OR,
    CMD_BACKGROUND
} cmd_type_t;

/* Redirection types */
typedef enum {
    REDIR_NONE,
    REDIR_IN,
    REDIR_OUT,
    REDIR_OUT_APPEND,
    REDIR_ERR,
    REDIR_ERR_APPEND,
    REDIR_BOTH,
    REDIR_BOTH_APPEND
} redir_type_t;

/* Job states */
typedef enum {
    JOB_RUNNING,
    JOB_STOPPED,
    JOB_DONE
} job_state_t;

/* Job structure */
typedef struct job {
    int job_id;
    pid_t pid;
    char *command;
    job_state_t state;
    int status;
    struct job *next;
} job_t;

/* Command structure */
typedef struct command {
    char **argv;
    int argc;
    redir_type_t redir_type;
    char *redir_file;
    struct command *next;
} command_t;

/* Variable structure */
typedef struct variable {
    char *name;
    char *value;
    struct variable *next;
} variable_t;

/* memoire partagée */
typedef struct {
    int initialized;
    int ref_count;
    sem_t readers_sem;
    sem_t writers_sem;
    sem_t mutex;
    int readers_count;
    int writers_waiting;
    char data[SHM_SIZE - 256];
} shared_env_t;

/* Variables globales */
extern job_t *job_list;
extern int job_counter;
extern int last_status;
extern char *last_command;
extern pid_t foreground_pid;
extern variable_t *local_vars;
extern int shmid;
extern shared_env_t *shared_env;

/* parser.c */
command_t *parse_command(char *line, cmd_type_t *type, int *background);
void free_command(command_t *cmd);

/* executor.c */
int execute_command(command_t *cmd, cmd_type_t type, int background);
int execute_pipeline(command_t *cmd);
int execute_simple_command(command_t *cmd);

/* builtins.c */
int is_builtin(char *cmd);
int execute_builtin(command_t *cmd);
int builtin_cd(char **argv);
int builtin_exit(char **argv);
int builtin_status(char **argv);
int builtin_myjobs(char **argv);
int builtin_myfg(char **argv);
int builtin_mybg(char **argv);

/* wildcards.c */
char **expand_wildcards(char **argv, int *argc);

/* redirections.c */
int setup_redirections(command_t *cmd);
void restore_redirections(int saved_stdin, int saved_stdout, int saved_stderr);

/* jobs.c */
job_t *add_job(pid_t pid, char *command);
void remove_job(int job_id);
job_t *get_job_by_id(int job_id);
job_t *get_job_by_pid(pid_t pid);
void check_background_jobs(void);
void print_jobs(void);
int get_highest_job_id(void);

/* variables.c */
int init_shared_env(char **envp);
void cleanup_shared_env(void);
char *get_variable(char *name);
void set_local_variable(char *name, char *value);
void unset_local_variable(char *name);
void set_env_variable(char *name, char *value);
void unset_env_variable(char *name);
char *expand_variables(char *str);
void print_local_variables(void);
void print_env_variables(void);
void lock_read_env(void);
void unlock_read_env(void);
void lock_write_env(void);
void unlock_write_env(void);

/* signals.c */
void setup_signals(void);
void sigchld_handler(int sig);
void sigint_handler(int sig);
void sigtstp_handler(int sig);

/* utils.c */
char *get_current_dir(void);
char *expand_tilde(char *path);
void print_prompt(void);
char *trim_whitespace(char *str);

#endif /* MYSH_H */
