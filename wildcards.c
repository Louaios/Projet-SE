#include "mysh.h"

static int match_pattern(const char *pattern, const char *str);
static int match_bracket(const char *pattern, char c, int *skip);

char **expand_wildcards(char **argv, int *argc) {
    glob_t globbuf;
    char **new_argv;
    int new_argc = 0;
    int i, j;
    
    new_argv = malloc(sizeof(char *) * MAX_ARGS);
    if (new_argv == NULL) {
        return argv;
    }
    
    for (i = 0; i < *argc; i++) {
        if (strchr(argv[i], '*') != NULL || 
            strchr(argv[i], '?') != NULL || 
            strchr(argv[i], '[') != NULL) {
            
            int has_unescaped = 0;
            for (j = 0; argv[i][j]; j++) {
                if ((argv[i][j] == '*' || argv[i][j] == '?' || argv[i][j] == '[') &&
                    (j == 0 || argv[i][j-1] != '\\')) {
                    has_unescaped = 1;
                    break;
                }
            }
            
            if (!has_unescaped) {
                new_argv[new_argc++] = strdup(argv[i]);
                continue;
            }
            
            int flags = GLOB_NOCHECK | GLOB_TILDE;
            if (glob(argv[i], flags, NULL, &globbuf) == 0) {
                for (j = 0; j < (int)globbuf.gl_pathc; j++) {
                    new_argv[new_argc++] = strdup(globbuf.gl_pathv[j]);
                }
                globfree(&globbuf);
            } else {
                new_argv[new_argc++] = strdup(argv[i]);
            }
        } else {
            new_argv[new_argc++] = strdup(argv[i]);
        }
    }
    
    new_argv[new_argc] = NULL;
    
    for (i = 0; i < *argc; i++) {
        free(argv[i]);
    }
    free(argv);
    
    *argc = new_argc;
    return new_argv;
}

static int match_bracket(const char *pattern, char c, int *skip) {
    int negate = 0;
    int match = 0;
    int i = 0;
    
    if (pattern[0] == '^') {
        negate = 1;
        i++;
    }
    
    while (pattern[i] && pattern[i] != ']') {
        if (pattern[i+1] == '-' && pattern[i+2] && pattern[i+2] != ']') {
            if (c >= pattern[i] && c <= pattern[i+2]) {
                match = 1;
            }
            i += 3;
        } else {
            if (c == pattern[i]) {
                match = 1;
            }
            i++;
        }
    }
    
    *skip = i + 1;  
    
    return negate ? !match : match;
}

static int match_pattern(const char *pattern, const char *str) {
    if (*pattern == '\0' && *str == '\0') {
        return 1;
    }
    
    if (*pattern == '\\' && *(pattern + 1)) {
        if (*(pattern + 1) == *str) {
            return match_pattern(pattern + 2, str + 1);
        }
        return 0;
    }
    
    if (*pattern == '*') {
        if (match_pattern(pattern + 1, str)) {
            return 1;
        }
        if (*str && match_pattern(pattern, str + 1)) {
            return 1;
        }
        return 0;
    }
    
    if (*pattern == '?') {
        if (*str == '\0') {
            return 0;
        }
        return match_pattern(pattern + 1, str + 1);
    }
    
    if (*pattern == '[') {
        int skip;
        if (match_bracket(pattern + 1, *str, &skip)) {
            return match_pattern(pattern + skip + 1, str + 1);
        }
        return 0;
    }
    
    if (*pattern == *str) {
        return match_pattern(pattern + 1, str + 1);
    }
    
    return 0;
}
