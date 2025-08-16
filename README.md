
This shell is designed for Linux systems and also runs on Windows via WSL. Other platforms are untested.

Mini Shell Usage
----------------

Build the shell:
  - gcc -Wall -o minishell bourne-shell/b-shell_main.c bourne-shell/b-shell_function.c
  - ./minishell

Commands to try:
- Run external programs: `ls`, `echo hello`
- Use pipelines: `echo hello | tr a-z A-Z`
<!--- - Search text: `grep pattern file` - Chain tools: `cat file | grep pattern | sort` --->
- Redirect output: `echo hi > out.txt`
- Redirect input: `cat < out.txt`
- Background jobs: `sleep 5 &`
- Builtins: `cd <dir>` and `exit`

Run tests:
  ./tests/run_tests.sh



Inspiration
-----------
This project takes cues from the original Bourne shell and educational resources on Unix programming. Implementing a shell from scratch clarifies how processes, pipelines, and file descriptors interact

Why build this shell?
---------------------
Crafting a minimal shell should help expose the mechanics behind linux os apis and build a core-foundation for secure remote shell project (future)

