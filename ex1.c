#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#define BUF_SIZE (100)
#define PATH_MAX (100)
#define ARGS_COUNT (64)
#define PROMPT "$ "
#define ERR_MSG "An error occurred\n"
#include <unistd.h>
#include <wait.h>
#include <pwd.h>

void cd (const char *path){
    // "cd ~" or "cd"
    if (path == NULL || strncmp(path, "~", 1) == 0) {
        uid_t uid = getuid();
        struct passwd* pwd = getpwuid(uid);
        chdir(pwd->pw_dir);
        return;
    }

    // if path starts with ..
    if (strncmp(path, "..", 2) == 0) {
        char bufferCwd[BUF_SIZE];
        // get the current cwd
        getcwd(bufferCwd, BUF_SIZE);
        char* last = strrchr(bufferCwd, '/');  // This cannot return NULL, always have / in path
        if (last != bufferCwd) { // not equal to start
            *last = '\0'; // cut string until last /
            if (chdir(bufferCwd) == -1) {
                printf("%s\n", "chdir failed");
                fflush(stdout);
            }
        } else {
            // *(last + 1) = '\0';
            chdir("/");
            // todo: if fail
            return;
        }

        char *first = strchr(path, '/');
        if (NULL == first) {
            return;
        }

        // not done - more path
        first++;
        cd(first);
    }

    // cd
    chdir(path);
}

void parse_string_echo(char *string) {
    for (int i = 0; i < strlen(string); ++i) {
        if (string[i] == '"') {
            memmove(string + i, string + i + 1, strlen(string + i + 1));
            (string + i)[strlen(string + i + 1)] = '\0';
            i--;
        }
    }
}

typedef struct process {
    pid_t pid; // -1 means done, otherwise check with waitpid
    char command[BUF_SIZE];
    bool is_background;
} process_t;

int main() {
    // while loop that scan input from the user.
    int cmd_indx = 0;
    char prev_path[PATH_MAX];
    process_t history[BUF_SIZE] = {0};

    while (true) {
        char buffer[BUF_SIZE] = {0};
        char *args[ARGS_COUNT] = {0};

//        {
//            char bufferCwd[BUF_SIZE];
//            getcwd(bufferCwd, BUF_SIZE);
//            //todo: delete in the end
//            printf("%s", bufferCwd);
//            fflush(stdout);
//        }

        printf("%s", PROMPT);
        fflush(stdout);

        unsigned int c;
        int idx = 0;
        int args_start_idx = 0;

        // read command
        do {
            if (idx >= BUF_SIZE) {
                printf(ERR_MSG);
                fflush(stdout);
            }
            // input from the user
            c = fgetc(stdin);
            history[cmd_indx].command[idx++] = (char) c;
            // parse args
            if (0 == args_start_idx && c == ' ') {
                args_start_idx = idx;
            }
        } while (c != '\n');

        history[cmd_indx].command[--idx] = '\0';
        memcpy(buffer, history[cmd_indx].command, BUF_SIZE);

        if (0 != args_start_idx) {
            buffer[args_start_idx - 1] = '\0';
        }

        // check if background
        if (buffer[--idx] == '&') {
            buffer[idx - 1] = '\0';
            history[cmd_indx].command[idx] = '\0';
            history[cmd_indx].is_background = true;
        }

        // create args array
        int args_idx = 1;
        args[0] = buffer; // convention (man exec)
        if (args_start_idx != 0) {
            args[args_idx++] = strtok(buffer + args_start_idx, " ");
            while (NULL != args[args_idx - 1]) {
                args[args_idx++] = strtok(NULL, " ");
            }
            args_idx--;
        }

        // handle builtin commands
        if (strcmp(args[0], "exit") == 0) {
            for (int i = 0; i < cmd_indx; i++) {
                if (history[i].is_background) {
                    int wstatus;
                    if (waitpid(history[i].pid, &wstatus, 0) == -1) {
                        printf(ERR_MSG);
                        fflush(stdout);
                }
            }
            return 0;
        }

        if (strcmp(args[0], "jobs") == 0) {
            history[cmd_indx].pid = -1;
            if (args_idx > 1) {
                printf(ERR_MSG);
                fflush(stdout);
                goto cont;
            }

            // pass all over the linked list and print
            for (int i = 0; i < cmd_indx + 1; i++) {
                int wstatus;
                if (history[i].is_background == true && 0 == waitpid(history[i].pid, &wstatus, WNOHANG)) {
                    printf("%s\n", history[i].command);
                    fflush(stdout);
                }
            }

            goto cont;
        }

        if (strcmp(args[0], "history") == 0) {
            history[cmd_indx].pid = -1;
            if (args_idx > 1) {
                printf(ERR_MSG);
                fflush(stdout);
                goto cont;
            }

            // pass all over the linked list and print
            for (int i = 0; i < cmd_indx + 1; i++) {
                printf("%s", history[i].command);
                fflush(stdout);
                int wstatus;
                if (-1 == history[i].pid ||
                    (history[i].pid != 0 && waitpid(history[i].pid, &wstatus, WNOHANG) != 0)) {
                    printf(" DONE\n");
                } else {
                    printf(" RUNNING\n");
                }
                fflush(stdout);
            }

            goto cont;
        }

        if (strcmp(args[0], "cd") == 0) {
            history[cmd_indx].pid = -1;
            if (args_idx > 2) {
                printf("Too many arguments\n");
                fflush(stdout);
                goto cont;
            }

            // cd ..
            // prev_path <- getcwd
            if (args[1] != NULL && strcmp(args[1], "-") == 0) {
                cd(prev_path);
            } else {
                getcwd(prev_path, PATH_MAX);
                cd(args[1]); // path starts at 3, after "cd "
            }

            goto cont;
        }

        if (strcmp(args[0], "echo") == 0) {
            parse_string_echo(args[1]);
            //char *ptr = strtok(args[1], '"');
            //printf(ptr);
        }

        // handle the other commands
        pid_t pid = fork();
        if (pid == -1) {
            printf("%s", "fork failed");
            fflush(stdout);
            goto cont;
        }

        if (pid == 0) {
            // in son
            if (-1 == execvp(buffer, args)) {
                printf("exec failed\n");
                fflush(stdout);
                return -1;
            }
        } else {
            // in parent
            history[cmd_indx].pid = pid;
            if (!history[cmd_indx].is_background) {
                // Foreground - *father wait*
                int wstatus;
                if (waitpid(pid, &wstatus, 0) == -1){
                    printf(ERR_MSG);
                    fflush(stdout);
                };
                // in the wstatus there is a value, it is irrelevant in the exercise
            }
        }
        cont:
        cmd_indx++;
    }

    return 0;
}