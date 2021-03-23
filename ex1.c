#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#define BUF_SIZE (8192)
#define PATH_MAX (1024)
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

int main() {
    // while loop that scan input from the user.
    char buffer[BUF_SIZE] = {0};
    char prev_path[PATH_MAX];
    char *args[ARGS_COUNT];
    while (true) {
        {
            char bufferCwd[BUF_SIZE];
            getcwd(bufferCwd, BUF_SIZE);
            printf("%s", bufferCwd);
            fflush(stdout);
        }
        printf("%s", PROMPT);
        fflush(stdout);

        unsigned int c;
        int idx = 0;
        int args_start_idx = 0;

        // read command
        do {
            if (idx >= BUF_SIZE) {
                // todo:
                // handle error
            }
            c = fgetc(stdin);
            buffer[idx++] = (char)c;

            if (0 == args_start_idx && c == ' ') {
                args_start_idx = idx;
            }
        } while (c != '\n');

        if (0 != args_start_idx) {
            buffer[args_start_idx - 1] = '\0';
        }

        buffer[--idx] = '\0';

        // create args array
        int args_idx = 0;
        args[args_idx++] = strtok(buffer + args_start_idx, " ");
        while (NULL != args[args_idx - 1]) {
            args[args_idx++] = strtok(NULL, " ");
        }

        if (0 == strncmp(buffer, "exit", 4)) {
            return 0;
        }

        if (strncmp(buffer, "jobs", 4) == 0) {
            // pass all over the linked list and print
        }

        if (strncmp(buffer, "history", 7) == 0) {
            // pass all over the linked list and print
        }

        // check all builtins
        // if cd  -> call cd

        if (strncmp(buffer, "cd", 2) == 0) {
            // cd ..
            // prev_path <- getcwd
            if (buffer[3] != '-') {
                getcwd(prev_path, PATH_MAX);
                cd(buffer + 3); // path starts at 3, after "cd "
            } else {
                cd(prev_path);
            }
        }

        if ((buffer[idx - 1]) == '&') {
            // father don't wait - fork  + exec
            fork();
            buffer[idx - 2]= '\0';
            const char delim[2] = "-";
            char *token = strtok(buffer, delim);
            // move to array
//            execv(token);
        }
        else {
            pid_t pid = fork();
            if (-1 == pid) {
                // todo
            }

            // father wait
            // fork + exec + waitpid
            if (0 == pid) {
                // in son
                execvp(buffer, args);
            } else {
                // in parent
                int wstatus;
                waitpid(pid, &wstatus, 0);
                // todo: check wstatus
            }
        }
    }
//        // else - not a builtin
//        // fork - exec - ...
//        pid_t pid = fork();
//        if (0 == pid) {
//            // in child
//            execv()
//        } else {
//            // in parent
//            // if foreground - wait for child
//            waitpid
//            // else - background - linked list
//        }
}