#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <cstdio>
#include <sys/wait.h>
#include <unistd.h>
#include <ctype.h>
#include <iostream>
#include "bash.h"

int flag = 0;

void cmdExecute(char **args) {
    // temporarily I'll send the pure text
    // in future I'll send the tokenize text for execution
    // cmdExecute(commandLine);

    char *cmd = args[0];

    if (strcmpr(cmd, "cd")) {
        changeDirectory(args[1]);
    }

    if (strcmpr(cmd, "apropos")) {

        findExecutionFileName(args[1]);
    }

    if (strcmpr(cmd, "history")) {

        if (strlen2(args) > 1) {
            if (strcmpr(args[1], "-c")) {
                clearHistory();
            }
        }

        else {
            readHistory();
        }
    }

    if (strcmpr(cmd, "color")) {
        // ==> need to create a help menu to show how to take input
        // color --bg red
        if (args[1] == NULL) {
            userManual();
            return;
        }
        else if (strcmpr(args[1], "--bg")) {
            if (args[2] == NULL) {
                printf("Color Code missing...\n");
                userManual();
                return;
            }
            char *bgCode = selectBGColor(args[2]);
            workingLoopWithColours(bgCode);
        }

        // color --fg user cyn
        else if (strcmpr(args[1], "--fg")) {
            if (args[3] == NULL || args[2] == NULL) {
                printf("Color Code missing...\n");
                userManual();
                return;
            }

            if (!strcmpr(args[2], "user") && !strcmpr(args[2], "host")) {
                printf("Wrong type declaration...\n");
                userManual();
                return;
            }

            char *fgCode = selectFGColor(args[3]);
            workingLoopWithColours(fgCode, args[2]);
        }

        else if (strcmpr(args[1], "--help")) {
            // printf("Wrong command...\n");
            userManual();
        }
    }
}

void change_directory(char *path) {
    char *tilde = "~";
    bool tildeStatus = strcmpr(tilde, path);
    char *newPath;

    if (tildeStatus) {
        newPath = strcatt("/home/", userName());
        printf("NEW: %s\n", newPath);

        if (chdir(newPath) != 0)
            printf("slsh: cd: %s: No such file or directory\n", newPath);
    }

    else if (strcontain(path, tilde)) {

        newPath = strcatt("/home/", userName());
        char **splittedWords = strsplit(tilde, path);
        char *finalPath = strcatt(newPath, splittedWords[1]);

        if (chdir(finalPath) != 0)
            printf("slsh: cd: %s: No such file or directory\n", newPath);
    }

    else {
        if (chdir(path) != 0)
            printf("slsh: cd: %s: No such file or directory\n", path);
    }
}

char *current_directory() {
    char cwd[1024], *addr;

    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        // copy the value of cwd into char pointer addr
        addr = cwd;
    }
    else {
        perror("getcwd() error");
        // return 1;
    }

    return addr;
}

char *hostName() {
    char *host = (char *)malloc(sizeof(char) * BUFFER_SIZE);

    FILE *fp;

    if ((fp = fopen("/etc/hostname", "r")) == NULL) {
        printf("Failed to open /etc/hostname\n");
    }

    // we need to use character array instead of pointers.
    // as pointer array of character lost the input after reading the whole file.
    char a[2000];

    while (fscanf(fp, "%s", a) != EOF) {
        host = a;
    }

    fclose(fp);

    return host;
}

char *userName() {
    // we need to use character array instead of pointers.
    // as pointer array of character lost the input after reading the whole file.
    char *currdir = current_directory();
    static char *user = (char *)malloc(sizeof(char) * BUFFER_SIZE);
    char *line = NULL;
    size_t length = 0;
    ssize_t read;
    FILE *fp;

    if ((fp = fopen("/etc/passwd", "r")) == NULL) {
        printf("Failed to open /etc/passwd\n");
    }

    // I need to use getline becuase it can handle line with spaces easily
    // other file function is malfunctioning in many cases
    // read - holds the length of the line
    while ((read = getline(&line, &length, fp)) != -1) {
        char **chunk = stringTokenize(line, ':');

        if (chunk[5] != 0) {
            // this following two condition is a must.
            // it's because if I don't add the first condition
            // it matches '/' with currentdir and that produce faulty results
            if (strcontain(chunk[5], "/home/") && strcontain(currdir, chunk[5])) {
                user = chunk[0];
            }
        }
    }

    fclose(fp);

    // if (line)
    // free(line);

    return user;
}

void execute(char **args) {

    // ==X this is the breakpoint for not working CD option
    char *command = args[0];

    if (strcmpr(command, "cd")) {

        cmdExecute(args);
    }

    else if (strcmpr(command, "apropos"))
    {

        if (strlen2(args) == 1)
        {
            printf("Provide argument after apropos..\napropos [options..]\n");
            return;
        }

        cmdExecute(args);
    }

    else if (strcmpr(command, "color"))
    {
        cmdExecute(args);
    }

    else if (strcmpr(command, "alias"))
    {
        aliasCommandsFunction(args);
    }

    else if (strcmpr(command, "history"))
    {
        cmdExecute(args);
    }

    else if (strcmpr(command, "autocomplete"))
    {
        if (strcmpr(args[1], "on"))
            flag = 1;
        else
            flag = 0;
    }

    // ==> will start from here
    // store process id
    // check if the process if child or parent
    // if it's the child process run execv()
    // if it's parent process then wait for the child process to be terminated
    else
    {

        pid_t status, process_id;

        process_id = fork();

        // child process
        if (process_id == 0)
        {
            if (execvp(args[0], args) == -1)
            {
                if (flag)
                    char *CMD = AutoCommandCompletion(flag, args[0]);
                else
                    commandSuggestion(flag, args);
            }
            exit(0);
        }
        else if (process_id < 1)
        {
            puts("Parent process");
            perror("Process Forking Failed\n");
        }

        else
        {
            status = waitpid(process_id, NULL, 0);
        }
    }
}

void executePipelinedCommandsFunction(int size, char *simpleCommand[], struct shellCommands command)
{

    pid_t pid[100];
    int fd[100][2];

    size = command.size;

    for (int i = 0; i < size; i++)
    {
        char *currCMD = strip(command.simpleCommands[i]);
        char **cmd = stringTokenize(currCMD, ' ');
        cmd = checkForWildCards(cmd); // checking if any wildcard pattern is available or not
        cmd = checkForAliasingFunction(cmd);

        int fdout, file;

        // this is input redirection...
        if (command.inputFile)
        {
            printf("READING>>>\n");
            int f2 = open(strip(command.inputFile), O_RDONLY, 0777);
            if (f2 == -1)
            {
                puts("Error reading file");
            }

            int fdin = dup2(f2, 0);
            close(f2);
        }

        // this is output redirection....
        if (command.outputFile)
        {
            printf("WRITING>>>\n");
            file = open(strip(command.outputFile), O_WRONLY | O_CREAT, 0777);
            if (file == -1)
            {
                puts("Error writing file");
            }

            // printf("Previous FD: %d\n", file);

            fdout = dup2(file, 1); // permanently converted the stdout...

            close(file);
        }

        if (i == 0)
        {
            // puts("FIRST");

            // pipe 1
            if (pipe(fd[i]) == -1)
            {
                printf("Error creating pipe;");
            }

            // cmd 1
            int x = fork();
            pid[i] = x;
            if (pid[i] == 0)
            {

                if (i != size - 1) // if we enter only a single command
                {
                    // child process; write end
                    dup2(fd[i][1], STDOUT_FILENO);
                    close(fd[i][1]);
                    close(fd[i][0]);
                }

                if (execvp(cmd[0], cmd) == -1)
                {
                    printf("Execution failed..#first: %d", i);
                }
            }
            
        }

        else if (i == size - 1)
        { // last cmd
          // cmd 4
            // puts("LAST");

            int x = fork();
            pid[i] = x;
            if (pid[i] == 0)
            {
                // child process; read end
                dup2(fd[i - 1][0], STDIN_FILENO);
                close(fd[i - 1][1]);
                close(fd[i - 1][0]);

                if (execvp(cmd[0], cmd) == -1)
                {
                    printf("Execution failed..#Last: %d", i);
                }
            }

            close(fd[i - 1][1]);
            close(fd[i - 1][0]);
            waitpid(pid[i], NULL, 0);
        }

        else
        {
            // puts("OTHERS");
            //  pipe 2
            if (pipe(fd[i]) == -1)
            {
                printf("Error creating pipe;");
            }

            // cmd 2
            int x = fork();
            pid[i] = x;
            if (pid[i] == 0)
            {
                // child process; read end
                dup2(fd[i - 1][0], STDIN_FILENO);
                close(fd[i - 1][1]);
                close(fd[i - 1][0]);

                if (i != size - 1)
                {
                    // // write end
                    dup2(fd[i][1], STDOUT_FILENO);
                    close(fd[i][1]);
                    close(fd[i][0]);
                }

                if (execvp(cmd[0], cmd) == -1)
                {
                    printf("Execution failed..#Middle: %d", i);
                }
            }

            // fd should be stopped here; otherwise it'll invoke to user keep the stdin / stdout open
            close(fd[i - 1][1]);
            close(fd[i - 1][0]);
            waitpid(pid[i], NULL, 0);
        }
    }

    return;
}

char *getCurrentDirectory()
{

    char *dir = current_directory();
    char *tilde = strcatt("/home/", userName());
    char *dirPath;

    if (strcontain(dir, tilde))
    {
        dirPath = strreplace(dir, tilde, "~");
    }
    else
    {
        dirPath = dir;
    }

    return dirPath;
}