#include "mysh.h"

/* Global variables */
job_t *job_list = NULL;
int job_counter = 0;
int last_status = 0;
char *last_command = NULL;
pid_t foreground_pid = -1;
variable_t *local_vars = NULL;
int shmid = -1;
shared_env_t *shared_env = NULL;

int main(int argc, char *argv[], char *envp[]) {
    char line[MAX_LINE];
    cmd_type_t type;
    int background;
    command_t *cmd;
    
    (void)argc;
    (void)argv;
    
    /* Initialisateur */
    if (init_shared_env(envp) < 0) {
        fprintf(stderr, "Failed to initialize shared environment\n");
        return 1;
    }
    
    /* Configuration des gestionnaires de signaux */
    setup_signals();
    
    /* Boucle principale */
    while (1) {
        /* Vérifie les tâches en arrière-plan */
        check_background_jobs();
        
        /* Affiche l'invite */
        print_prompt();
        
        /* Lit la commande */
        if (fgets(line, MAX_LINE, stdin) == NULL) {
            if (feof(stdin)) {
                printf("\n");
                break;
            }
            continue;
        }
        
        /* Supprime le saut de ligne final */
        line[strcspn(line, "\n")] = '\0';
        
        /* Ignore les lignes vides */
        if (strlen(trim_whitespace(line)) == 0) {
            continue;
        }
        
        /* Parse la commande */
        cmd = parse_command(line, &type, &background);
        if (cmd == NULL) {
            continue;
        }
        
        /* Exécute la commande */
        execute_command(cmd, type, background);
        
        /* Libère la structure de commande */
        free_command(cmd);
    }
    
    /* Nettoyage */
    cleanup_shared_env();
    
    return 0;
}
