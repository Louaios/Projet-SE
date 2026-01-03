#include "mysh.h"

char *get_current_dir(void) {
    static char cwd[MAX_LINE];
    char *home;
    
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        return "?";
    }
    
    /* remplacer le répertoire personnel par ~ */
    home = getenv("HOME");
    if (home != NULL) {
        size_t home_len = strlen(home);
        if (strncmp(cwd, home, home_len) == 0) {
            static char result[MAX_LINE];
            snprintf(result, sizeof(result), "~%s", cwd + home_len);
            return result;
        }
    }
    
    return cwd;
}

char *expand_tilde(char *path) {
    static char expanded[MAX_LINE];
    char *home;
    
    if (path[0] != '~') {
        return path;
    }
    
    home = getenv("HOME");
    if (home == NULL) {
        return path;
    }
    
    snprintf(expanded, sizeof(expanded), "%s%s", home, path + 1);
    return expanded;
}

void print_prompt(void) {
    printf("%s> ", get_current_dir());
    fflush(stdout);
}

char *trim_whitespace(char *str) {
    char *end;
    
    while (*str == ' ' || *str == '\t') {
        str++;
    }
    
    if (*str == 0) {
        return str;
    }
    
    end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t')) {
        end--;
    }
    
    *(end + 1) = '\0';
    
    return str;
}
