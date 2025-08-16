#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include "bourne.h"
#ifndef HAVE_STRDUP
char *strdup(const char *s) {
    size_t len = strlen(s) + 1;
    char *p = malloc(len);
    if (p) {
        memcpy(p, s, len);
    }
    return p;
}
#endif

void print_prompt(void) {
    printf("myshell> ");
    fflush(stdout);
}

int is_background(char *cmd) {
    char *last = strchr(cmd, '\0');
    if (last > cmd && *(last - 1) == '&') {
        *(last - 1) = '\0';
        return 1;
    }
    return 0;
}

char *get_input_redirection(char *cmd) {
    char *ptr = strchr(cmd, '<');
    if (!ptr) return NULL;

    *ptr = '\0';
    char *file = ptr + 1;
    while (*file == ' ') file++; // Skip leading spaces
    char *end = strchr(file, '\0');
    while (end > file && *(end - 1) == ' ') end--; // Skip trailing spaces

    char *result = malloc(end - file + 1);
    strncpy(result, file, end - file);
    result[end - file] = '\0';
    return result;
}

char *get_output_redirection(char *cmd) {
    char *ptr = strchr(cmd, '>');
    if (!ptr) return NULL;

    *ptr = '\0';
    char *file = ptr + 1;
    while (*file == ' ') file++;
    char *end = strchr(file, '\0');
    while (end > file && *(end - 1) == ' ') end--;

    char *result = malloc(end - file + 1);
    strncpy(result, file, end - file);
    result[end - file] = '\0';
    return result;
}

char **split_commands(char *cmd_line, int *num_cmds) {
    char **commands = malloc(MAX_ARGS * sizeof(char *));
    char *ptr = cmd_line;
    *num_cmds = 0;

    char *pipe = strchr(ptr, '|');
    while (pipe) {
        commands[*num_cmds] = malloc(pipe - ptr + 1);
        strncpy(commands[*num_cmds], ptr, pipe - ptr);
        commands[*num_cmds][pipe - ptr] = '\0';
        (*num_cmds)++;
        ptr = pipe + 1;
        pipe = strchr(ptr, '|');
    }

    char *end = strchr(ptr, '\0');
    commands[*num_cmds] = malloc(end - ptr + 1);
    strncpy(commands[*num_cmds], ptr, end - ptr);
    commands[*num_cmds][end - ptr] = '\0';
    (*num_cmds)++;

    return commands;
}

int is_terminated(const char *cmd) {
    return (cmd && (strcmp(cmd, "exit") == 0 || strcmp(cmd, "quit") == 0));
}

char **parse_args(char *cmd) {
    char **args = malloc(MAX_ARGS * sizeof(char *));
    int i = 0;

    char *token = strtok(cmd, " \t");
    while (token && i < MAX_ARGS - 1) {
        args[i] = malloc(strlen(token) + 1);
        strcpy(args[i], token);
        i++;
        token = strtok(NULL, " \t");
    }
    args[i] = NULL; // Null-terminate the array
    return args;
}

char *find_command_path(const char *path, const char *cmd) {
    char *path_copy = strdup(path);
    char *full_path = malloc(MAX_PATH_LEN + MAX_ARG_LEN + 2); // +2 for '/' and '\0'

    char *dir = strtok(path_copy, ":");
    while (dir) {
        snprintf(full_path, MAX_PATH_LEN + MAX_ARG_LEN + 2, "%s/%s", dir, cmd);
        if (access(full_path, X_OK) == 0) {
            free(path_copy);
            return full_path;
        }
        dir = strtok(NULL, ":");
    }

    free(path_copy);
    free(full_path);
    return NULL;
}

int setup_input_redirection(const char *file) {
    int fd = open(file, O_RDONLY);
    if (fd == -1) {
        perror("open input file");
        return -2;
    }
    int saved_stdin = dup(STDIN_FILENO);
    dup2(fd, STDIN_FILENO);
    close(fd);
    return saved_stdin;
}

int setup_output_redirection(const char *file) {
    int fd = open(file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open output file");
        return -2;
    }
    int saved_stdout = dup(STDOUT_FILENO);
    dup2(fd, STDOUT_FILENO);
    close(fd);
    return saved_stdout;
}

void free_args(char **args) {
    if (!args) return;
    for (int i = 0; args[i]; i++) {
        free(args[i]);
    }
    free(args);
}

void free_commands(char **commands, int num_cmds) {
    if (!commands) return;
    for (int i = 0; i < num_cmds; i++) {
        free(commands[i]);
    }
    free(commands);
}
