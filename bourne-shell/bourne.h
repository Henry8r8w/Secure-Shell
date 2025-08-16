#ifndef MYSHELL_H
#define MYSHELL_H

#define LINE_LEN 80
#define MAX_ARGS 64
#define MAX_ARG_LEN 64
#define MAX_PATHS 64
#define MAX_PATH_LEN 96

void print_prompt(void);
int is_background(char *cmd);
char *get_input_redirection(char *cmd);
char *get_output_redirection(char *cmd);
// split input line into commands (for piping)
char **split_commands(char *cmd_line, int *num_cmds);
// check if command is "exit" or "quit"
int is_terminated(const char *cmd);
char **parse_args(char *cmd);
char *find_command_path(const char *path, const char *cmd);
int setup_input_redirection(const char *file);
int setup_output_redirection(const char *file);
void free_args(char **args);
void free_commands(char **commands, int num_cmds);

#endif
