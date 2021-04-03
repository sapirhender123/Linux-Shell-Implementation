// Sapir Hender 208414573
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
#include <stdlib.h>

void format_home_dir(char wanted_dir[], const char *path)
{
    uid_t uid = getuid();
    struct passwd* pwd = getpwuid(uid);
    memcpy(wanted_dir, pwd->pw_dir, strlen(pwd->pw_dir)); // /home/<user>
    wanted_dir[strlen(pwd->pw_dir)] = '/';
    if (path != NULL && strlen(path) > 2) {
        memcpy(wanted_dir + strlen(pwd->pw_dir) + 1, path + 2, strlen(path + 2)); // /home/<user>/<path>
    }
}

void cd (const char *path){
    // "cd ~" or "cd"
    // Home
    if (path == NULL || strcmp(path, "~") == 0 || strncmp(path, "~/", 2) == 0) {
        char wanted_dir[100] = {0};
        format_home_dir(wanted_dir, path);

        if (-1 == chdir(wanted_dir)) {
            printf("chdir failed\n");
            fflush(stdout);
        }

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
            if (chdir("/") == -1) {
                printf("%s\n", "chdir failed");
                fflush(stdout);
            }
            return;
        }

        char *first = strchr(path, '/');
        if (NULL == first || 0 == strcmp(first, "/")) {
            return;
        }

        // not done - more path
        first++;
        cd(first);
    }

    // cd
    if (-1 == chdir(path)) {
        printf("chdir failed\n");
        fflush(stdout);
    }
}

void parse_string_echo(char *string) {
    int i = 0;
    for (; i < strlen(string); ++i) {
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
    char prev_path[PATH_MAX] = {0};
    getcwd(prev_path, PATH_MAX);

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
            buffer[idx] = '\0';
            history[cmd_indx].command[idx] = '\0';
            history[cmd_indx].command[idx - 1] = '\0';
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
            exit(0);
        }

        if (strcmp(args[0], "jobs") == 0) {
            history[cmd_indx].pid = -1;
            if (args_idx > 1) {
                printf(ERR_MSG);
                fflush(stdout);
                goto cont;
            }

            // pass all over the linked list and print
            int i = 0;
            for (; i < cmd_indx + 1; i++) {
                int wstatus;
                if (history[i].is_background == true && 0 == waitpid(history[i].pid, &wstatus, WNOHANG)) {
                    printf("%s\n", history[i].command);
                    fflush(stdout);
                }
            }

            goto cont;
        }

        if (strcmp(args[0], "history") == 0) {
            // todo:check

            // the history have no args
            if (args_idx > 1) {
                printf(ERR_MSG);
                fflush(stdout);
                goto cont;
            }

            // pass all over the linked list and print
            int i = 0;
            for (; i < cmd_indx + 1; i++) {
                printf("%s", history[i].command);
                fflush(stdout);
                int wstatus;

                /* It should be in the DONE commands when it is one of the following reasons:
                 * 1. if there was a command with error
                 * 2. if there are no children for the process
                 * 3. if the command is a built-in command
                 */
                if (-1 == history[i].pid ||
                    (history[i].pid != 0 && waitpid(history[i].pid, &wstatus, WNOHANG) != 0)) {
                    printf(" DONE\n");
                } else {
                    printf(" RUNNING\n");
                }
                fflush(stdout);
            }

            history[cmd_indx].pid = -1;
            goto cont;
        }

        if (strcmp(args[0], "cd") == 0) {
            history[cmd_indx].pid = -1;
            if (args_idx > 2) {
                printf("Too many arguments\n");
                fflush(stdout);
                goto cont;
            }

            // prev_path <- getcwd
            if (args[1] != NULL && strcmp(args[1], "-") == 0) {
                char tmp[100];
                strcpy(tmp, prev_path);
                getcwd(prev_path, PATH_MAX);
                cd(tmp);
            } else {
                // path is NULL or path != "-"
                getcwd(prev_path, PATH_MAX);
                cd(args[1]); // arg[1] is the path
            }

            goto cont;
        }

        if (strcmp(args[0], "echo") == 0) {
            parse_string_echo(args[1]);
        }

        // handle the other commands
        pid_t pid = fork();
        if (pid == -1) {
            printf("%s", "fork failed\n");
            fflush(stdout);
            goto cont;
        }

        if (pid == 0) {
            // in son
            // handle if the first argument starts with ~/, format home directory instead
            if (args[1] != NULL && strncmp(args[1], "~/", 2) == 0) {
                char wanted_dir[100] = {0};
                format_home_dir(wanted_dir, args[1]);
                args[1] = wanted_dir;
            }

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