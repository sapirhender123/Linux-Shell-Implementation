#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#define BUF_SIZE (100)
#define PATH_MAX (100)
#define ARGS_COUNT (64)
#define PROMPT "$ "
#include <unistd.h>
#include <wait.h>
#include <pwd.h>

//-~, ..-, ..~, ~.., -..
void cd (const char *path){
    // if path starts with ..
    if (strncmp(path, "..", 2) == 0) {
        char bufferCwd[BUF_SIZE];
        // get the current cwd
        getcwd(bufferCwd, BUF_SIZE);
        char* last = strrchr(bufferCwd, '/');  // This cannot return NULL, always have / in path
        if (last != bufferCwd) { // not equal to start
            *last = '\0'; // cut string until last /
            if (chdir(bufferCwd) == -1) {
                printf("%s \n", "chdir failed");
            }
        } else {
            // *(last + 1) = '\0';
            chdir("/");
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

    // cd ~
    if (strncmp(path, "~", 1) == 0) {
        uid_t uid = getuid();
        struct passwd* pwd = getpwuid(uid);
        chdir(pwd->pw_dir);
        return;
    }

    // cd
    chdir(path);
}

typedef struct process {
    pid_t pid;
    char command[BUF_SIZE];
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
                printf("An error occurred");
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
        bool is_background = false;
        if (buffer[--idx] == '&') {
            is_background = true;
            buffer[--idx] = '\0';
            history[cmd_indx].command[idx] = '\0';
        }

        // create args array
        int args_idx = 1;
        args[0] = buffer; // convention (man exec)
        if (args_start_idx != 0) {
            args[args_idx++] = strtok(buffer + args_start_idx, " ");
            while (NULL != args[args_idx - 1]) {
                args[args_idx++] = strtok(NULL, " ");
            }
        }

        // handle builtin commands
        if (0 == strncmp(buffer, "exit", 4)) {
            return 0;
        }

        if (strncmp(buffer, "jobs", 4) == 0) {
            // pass all over the linked list and print
        }

        if (strncmp(buffer, "history", 7) == 0) {
            // pass all over the linked list and print
            for (int i = 0; i < cmd_indx + 1; i++) {
                printf("%s", history[i].command);
                int wstatus;
                if (waitpid(history[i].pid, &wstatus, WNOHANG) != 0) {
                    printf(" DONE\n");
                } else {
                    printf(" RUNNING\n");
                }
                fflush(stdout);
            }
            continue;
        }

        if (strncmp(buffer, "cd", 2) == 0) {
            // cd ..
            // prev_path <- getcwd
            if (buffer[3] != '-') {
                getcwd(prev_path, PATH_MAX);
                cd(buffer + 3); // path starts at 3, after "cd "
            } else {
                cd(prev_path);
            }
            continue;
        }

        // handle the other commands
        pid_t pid = fork();
        if (pid == -1) {
            printf("%s", "fork failed");
            fflush(stdout);
            continue;
        }

        if (pid == 0) {
            // in son
            execvp(buffer, args);
        } else {
            // in parent
            history[cmd_indx].pid = pid;
            if (is_background) {
                // Background - father don't wait - fork  + exec
                // add to list
            } else {
                // Foreground - *father wait*
                // fork + exec + waitpid
                int wstatus;
                waitpid(pid, &wstatus, 0);
                // in the wstatus there is a value, it is irrelevant in the exercise
                //man no hang
            }
        }

        cmd_indx++;
    }
}