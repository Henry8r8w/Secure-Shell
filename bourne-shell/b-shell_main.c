#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include "bourne.h"
extern char **environ;

int main(void) {
    char cmd_line[LINE_LEN];
    char *path = getenv("PATH");

    while (1) {
        print_prompt();
        if (fgets(cmd_line, LINE_LEN, stdin) == NULL) {
            break;
        }
        cmd_line[strcspn(cmd_line, "\n")] = '\0';
        int num_cmds = 0;
        char **commands = split_commands(cmd_line, &num_cmds);
        if (num_cmds == 0) {
            free_commands(commands, num_cmds);
            continue;}

        int fd[2];
        if (num_cmds > 1) {
            if (pipe(fd) == -1) {
                perror("pipe");
                free_commands(commands, num_cmds);
                continue;
            }
        }

        pid_t *pids = malloc(num_cmds * sizeof(pid_t));
        int *bg_flags = calloc(num_cmds, sizeof(int));
        for (int i = 0; i < num_cmds; i++) {
            int is_bg = is_background(commands[i]);
            bg_flags[i] = is_bg;
            char *input_file = get_input_redirection(commands[i]);
            char *output_file = get_output_redirection(commands[i]);
            char **args = parse_args(commands[i]);

            if (is_terminated(args[0])) {
                free_args(args);
                free(input_file);
                free(output_file);
                free_commands(commands, num_cmds);
                exit(0);
            }

            // Handle built-in 'cd' command
            if (strcmp(args[0], "cd") == 0) {
                if (args[1] == NULL) {
                    fprintf(stderr, "cd: missing argument\n");
                } else if (chdir(args[1]) == -1) {
                    perror("cd");
                }
                free_args(args);
                free(input_file);
                free(output_file);
                continue;
            }

            char *full_path = find_command_path(path, args[0]);
            if (!full_path) {
                fprintf(stderr, "%s: command not found\n", args[0]);
                free_args(args);
                free(input_file);
                free(output_file);
                continue;
            }

            int saved_stdin = -1, saved_stdout = -1;
            if (input_file) {
                saved_stdin = setup_input_redirection(input_file);
                if (saved_stdin == -2) {
                    free(full_path);
                    free_args(args);
                    free(input_file);
                    free(output_file);
                    continue;
                }
            }
            if (output_file) {
                saved_stdout = setup_output_redirection(output_file);
                if (saved_stdout == -2) {
                    if (saved_stdin != -1) {
                        dup2(saved_stdin, STDIN_FILENO);
                        close(saved_stdin);
                    }
                    free(full_path);
                    free_args(args);
                    free(input_file);
                    free(output_file);
                    continue;
                }
            }

            pid_t pid = fork();
            if (pid == -1) {
                perror("fork");
            } else if (pid == 0) {
                if (num_cmds > 1) {
                    if (i == 0) {
                        close(fd[0]);
                        dup2(fd[1], STDOUT_FILENO);
                        close(fd[1]);
                    } else {
                        close(fd[1]);
                        dup2(fd[0], STDIN_FILENO);
                        close(fd[0]);
                    }
                }

                if (execve(full_path, args, environ) == -1) {
                    perror("execve");
                    exit(1);
                }
            } else {
                pids[i] = pid;
            }

            free(full_path);
            free_args(args);
            free(input_file);
            free(output_file);

            if (saved_stdin != -1) {
                dup2(saved_stdin, STDIN_FILENO);
                close(saved_stdin);
            }
            if (saved_stdout != -1) {
                dup2(saved_stdout, STDOUT_FILENO);
                close(saved_stdout);
            }
        }

        if (num_cmds > 1) {
            close(fd[0]);
            close(fd[1]);
        }

        for (int i = 0; i < num_cmds; i++) {
            if (!bg_flags[i]) {
                int status;
                waitpid(pids[i], &status, 0);
            }
        }

        free(bg_flags);
        free(pids);
        free_commands(commands, num_cmds);
    }

    return 0;
}

