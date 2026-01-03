#include "mysh.h"

static command_t *create_command(void) {
    command_t *cmd = malloc(sizeof(command_t));
    if (cmd == NULL) {
        perror("malloc");
        return NULL;
    }
    
    cmd->argv = malloc(sizeof(char *) * MAX_ARGS);
    if (cmd->argv == NULL) {
        perror("malloc");
        free(cmd);
        return NULL;
    }
    
    cmd->argc = 0;
    cmd->redir_type = REDIR_NONE;
    cmd->redir_file = NULL;
    cmd->next = NULL;
    
    return cmd;
}

void free_command(command_t *cmd) {
    command_t *current = cmd;
    command_t *next;
    
    while (current != NULL) {
        next = current->next;
        
        if (current->argv != NULL) {
            for (int i = 0; i < current->argc; i++) {
                free(current->argv[i]);
            }
            free(current->argv);
        }
        
        if (current->redir_file != NULL) {
            free(current->redir_file);
        }
        
        free(current);
        current = next;
    }
}

static int is_operator(char *token) {
    return (strcmp(token, "|") == 0 ||
            strcmp(token, ";") == 0 ||
            strcmp(token, "&&") == 0 ||
            strcmp(token, "||") == 0 ||
            strcmp(token, "&") == 0 ||
            strcmp(token, ">") == 0 ||
            strcmp(token, ">>") == 0 ||
            strcmp(token, "2>") == 0 ||
            strcmp(token, "2>>") == 0 ||
            strcmp(token, ">&") == 0 ||
            strcmp(token, ">>&") == 0 ||
            strcmp(token, "<") == 0);
}

static char *get_next_token(char **line_ptr) {
    char *line = *line_ptr;
    static char token[MAX_LINE];
    int i = 0;
    int in_quotes = 0;
    int escape = 0;
    
    while (*line && (*line == ' ' || *line == '\t')) {
        line++;
    }
    
    if (*line == '\0') {
        *line_ptr = line;
        return NULL;
    }
    
    if (strncmp(line, "2>>", 3) == 0) {
        strcpy(token, "2>>");
        *line_ptr = line + 3;
        return token;
    }
    if (strncmp(line, ">>&", 3) == 0) {
        strcpy(token, ">>&");
        *line_ptr = line + 3;
        return token;
    }
    if (strncmp(line, ">>", 2) == 0) {
        strcpy(token, ">>");
        *line_ptr = line + 2;
        return token;
    }
    if (strncmp(line, "2>", 2) == 0) {
        strcpy(token, "2>");
        *line_ptr = line + 2;
        return token;
    }
    if (strncmp(line, ">&", 2) == 0) {
        strcpy(token, ">&");
        *line_ptr = line + 2;
        return token;
    }
    if (strncmp(line, "&&", 2) == 0) {
        strcpy(token, "&&");
        *line_ptr = line + 2;
        return token;
    }
    if (strncmp(line, "||", 2) == 0) {
        strcpy(token, "||");
        *line_ptr = line + 2;
        return token;
    }
    if (*line == '|' || *line == '&' || *line == ';' || *line == '<' || *line == '>') {
        token[0] = *line;
        token[1] = '\0';
        *line_ptr = line + 1;
        return token;
    }
    
    while (*line && (in_quotes || (!is_operator(line) && *line != ' ' && *line != '\t'))) {
        if (escape) {
            token[i++] = *line;
            escape = 0;
        } else if (*line == '\\') {
            escape = 1;
        } else if (*line == '"' || *line == '\'') {
            in_quotes = !in_quotes;
        } else {
            token[i++] = *line;
        }
        line++;
    }
    
    token[i] = '\0';
    *line_ptr = line;
    
    return token;
}

command_t *parse_command(char *line, cmd_type_t *type, int *background) {
    command_t *first_cmd = NULL;
    command_t *current_cmd = NULL;
    char *line_ptr = line;
    char *token;
    int expect_file = 0;
    redir_type_t pending_redir = REDIR_NONE;
    
    *type = CMD_SIMPLE;
    *background = 0;
    
    char *expanded = expand_variables(line);
    if (expanded != NULL) {
        line_ptr = expanded;
    }
    
    current_cmd = create_command();
    if (current_cmd == NULL) {
        return NULL;
    }
    first_cmd = current_cmd;
    
    while ((token = get_next_token(&line_ptr)) != NULL) {
        if (expect_file) {
            current_cmd->redir_file = strdup(token);
            expect_file = 0;
            continue;
        }
        
        if (strcmp(token, ">") == 0) {
            pending_redir = REDIR_OUT;
            expect_file = 1;
        } else if (strcmp(token, ">>") == 0) {
            pending_redir = REDIR_OUT_APPEND;
            expect_file = 1;
        } else if (strcmp(token, "2>") == 0) {
            pending_redir = REDIR_ERR;
            expect_file = 1;
        } else if (strcmp(token, "2>>") == 0) {
            pending_redir = REDIR_ERR_APPEND;
            expect_file = 1;
        } else if (strcmp(token, ">&") == 0) {
            pending_redir = REDIR_BOTH;
            expect_file = 1;
        } else if (strcmp(token, ">>&") == 0) {
            pending_redir = REDIR_BOTH_APPEND;
            expect_file = 1;
        } else if (strcmp(token, "<") == 0) {
            pending_redir = REDIR_IN;
            expect_file = 1;
        } else if (strcmp(token, "|") == 0) {
            *type = CMD_PIPE;
            current_cmd->redir_type = pending_redir;
            pending_redir = REDIR_NONE;
            
            command_t *new_cmd = create_command();
            if (new_cmd == NULL) {
                free_command(first_cmd);
                return NULL;
            }
            current_cmd->next = new_cmd;
            current_cmd = new_cmd;
        } else if (strcmp(token, ";") == 0) {
            if (*type != CMD_PIPE) {
                *type = CMD_SEQUENCE;
            }
        } else if (strcmp(token, "&&") == 0) {
            if (*type != CMD_PIPE) {
                *type = CMD_AND;
            }
        } else if (strcmp(token, "||") == 0) {
            if (*type != CMD_PIPE) {
                *type = CMD_OR;
            }
        } else if (strcmp(token, "&") == 0) {
            *background = 1;
        } else {
            current_cmd->argv[current_cmd->argc++] = strdup(token);
        }
        
        if (expect_file) {
            current_cmd->redir_type = pending_redir;
        }
    }
    
    for (command_t *cmd = first_cmd; cmd != NULL; cmd = cmd->next) {
        cmd->argv[cmd->argc] = NULL;
        
        cmd->argv = expand_wildcards(cmd->argv, &cmd->argc);
    }
    
    if (expanded != NULL) {
        free(expanded);
    }
    
    return first_cmd;
}
