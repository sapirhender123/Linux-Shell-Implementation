# Linux Shell Implementation

This project includes Shell implementation:
The program displays a PROMPT cursor on the screen and allows the user to type commands in "Linux".

There are two types of commands that are implemented:
1. **Foreground** - that is, the shell process I wrote reads the command typed by the user and creates a child process to execute it. The parent process will wait for the child process to finish before continuing to read more commands from the user.
2. **Backround** - also in this case a child process will be created, but will run in the background without the parent process waiting for it.
A new prompt will be displayed, for the purpose of receiving a new command from the user, regardless of whether the child process has already ended or not.

For non-built-in commands, the shell I wrote will not try to understand the commands entered by the user, but will pass the command and arguments to the system by calling a function from the exec family.
Below is a breakdown of the built-in commands that the program supports:
1. **jobs** - the list of commands currently running in the background according to the chronological order of their entry
2. **history** - all the commands the user entered during the program's run, in chronological order from the earliest to the latest, including commands that failed.
3. **cd** - changes the current directory of the process. cd also supports **3 flags: -, .., ~**
4. **exit** - leaving the program
