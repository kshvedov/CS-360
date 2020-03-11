#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>


char line[1024];
char command[16], rCommands[1024];
char *names[1024] = { NULL };
char *paths[1024] = { NULL };
char *home;
size_t size;
char *buf;
char *curWD;

//function decelerations
void readInput(void);
void getInput(void);
int tokenize(char *input);
int runCommand(char *env[]);

int cd(char *string);
int forkChild(char *env[]);
int exec(char **input, char* env[]);
void execPipe(int pipeNum, char** input, char* env[]);

//color functions
void red(void);
void green(void);
void blue();
void back(void);
void fileColour(void);
void dirColour(void);

int main(int argc, char *argv[ ], char *env[ ])
{
    home = getenv("HOME");
    printf("Home DIR: %s\n", home);
    strcpy(command, "empty");
    while(strcmp(command, "exit") != 0)
    {
        printf("*************************************************************\n");
        readInput();
        printf("Command: %s, the rest: ", command);
        for (int i = 1; names[i] != NULL; i++)
            printf("%s ", names[i]);

        printf("\n\n");
        if(strcmp(command, "exit") != 0)
        {
            runCommand(env);
        }
    }
    printf("Ending The Program\n");
    return 0;
}

//reads input from the user
void readInput(void)
{
    curWD = getcwd(buf,size);
    fileColour();
    printf("\nUser@KSsh");
    back();
    printf(":");
    dirColour();
    printf("~%s$ ", curWD);
    back();
    getInput();
    tokenize(line);
}

//gets input and parses the first part from the rest
void getInput(void)
{
    for(int i = 0; i < 16; i++)
        command[i]='\0';
    command[0] = rCommands[0] = '\0';
    fgets(line, 1024, stdin); //retrieves 128 at max
    line[strlen(line) - 1] = 0; //removes \n at end of the line
    sscanf(line, "%s %s", command, rCommands);
}

//tokenize's the rest of the code
int tokenize(char *input)
{
    for (int i = 0; i < 1024; i++)    //sets all values in array to null so a new path can be interpreted
    {
        names[i] = NULL;
    }
    names[0] = strtok(input, " "); // first call to strtok()
    int i = 1;
    for (; names[i - 1]; i++)
    {
        names[i] = strtok(0, " "); // calling strtok() until it returns NULL
    }
}

//run the commands
int runCommand(char *env[])
{
    // If command = cd run cd command
    if(strcmp(command, "cd") == 0)
    {
        return cd(names[1]);
    }
    else
    {
        return forkChild(env);
    }
}

//changes DIR
int cd(char *path)
{
    if (strcmp(rCommands,"") == 0)
    {
        if (home)
        {
            chdir(home);
        }
        else return 0;
    }
    else
        if (chdir(rCommands) == -1) {
            printf("Directory doesnt exist\n");
            return 0;
        }
    return 1;
}

//this function is in charge of forking the child
int forkChild(char *env[])
{
    red();
    printf("INFORK CHILD\n");
    back();
    int pid, status = 0, pipes = 0;


    pid = fork();
    green();
    printf("PID = %d\n", pid);
    back();
    if(pid < 0)
    {
        red();
        printf ("Couldn't fork child\n");
        back();
        return 0;
    }
    else if(pid == 0) //child executed
    {
        red();
        printf("CHILD %d executed\n", getpid());
        back();
        for(int i = 0; names[i] != NULL; i++)
        {
            if(strcmp(names[i],"|") == 0)
            {
                pipes++;
            }
        }
        green();
        printf("Pipes found %d\n", pipes);
        back();

        if(pipes > 0)
        {
            execPipe(pipes, names, env);
        }
        else
        {
            status = exec(names, env);
        }

        red();
        printf("child %d dies by exit(VALUE)\n", getpid());
        back();
        exit(100);
    }
    else //parent executed
    {
        red();
        printf("PARENT %d WAITS FOR CHILD %d TO DIE\n", getpid(),pid);
        back();
        pid=wait(&status); // wait for ZOMBIE child process
        red();
        printf("DEAD CHILD=%d, status=0x%04x\n", pid, status);
        back();
    }
    return status;
}


int exec(char **input, char* env[])
{
    printf("\n\n");
    /***********************************************************************************/
    /***********************************************************************************/
    /***********************************************************************************/
    for(int i = 0; input[i] != NULL; i++)
    {
        if (strcmp(input[i], "<") == 0) // input
        {
            close(0);
            open(input[i + 1], O_RDONLY);
            input[i] = NULL;
        }
        else if (strcmp(input[i], ">") == 0) // output
        {
            close(1);
            open(input[i + 1], O_WRONLY | O_CREAT, 0644);
            input[i] = NULL;
        }
        else if (strcmp(input[i], ">>") == 0) // output and append
        {
            close(1);
            open(input[i + 1], O_WRONLY | O_APPEND);
            input[i] = NULL;
        }
    }
    /***********************************************************************************/
    /***********************************************************************************/
    /***********************************************************************************/

    char *path = getenv("PATH");
    for (int i = 0; i < 1024; i++)    //sets all values in array to null so a new path can be interpreted
    {
        paths[i] = NULL;
    }
    paths[0] = strtok(path, ":"); // first call to strtok()
    int i = 1;
    for (; paths[i - 1]; i++)
    {
        paths[i] = strtok(0, ":"); // calling strtok() until it returns NULL
    }
    int r = 0;
    char cmd[1024];
    for(int j = 0; paths[j] != NULL; j++) {
        strcpy(cmd, paths[j]);
        strcat(cmd, "/");
        strcat(cmd, input[0]);
        //printf("Attempting CMD: %s\n", cmd);
        r = execve(cmd, input, env);
    }
    printf("execve() failed: r = %d\n", r); // only if fails
}

void execPipe(int pipeNum, char** input, char* env[])
{
    char *left[1024] = { NULL };
    char *right[1024] = { NULL };
    int i = 0;
    int j = 0;

    while(strcmp(input[i], "|") != 0)
    {
        left[i] = input[i];
        i++;
    }
    i++;
    while(input[i] != NULL)
    {
        right[j] = input[i];
        i++;
        j++;
    }

    green();
    printf("Left Side: ");
    for (int k = 0; left[k] != NULL; k++)
    {
        printf("%s ", left[k]);
    }
    printf("\n");

    printf("Right Side: ");
    for (int k = 0; right[k] != NULL; k++)
    {
        printf("%s ", right[k]);
    }
    printf("\n");
    back();

    pipeNum--;

    int pd[2], pid;
    pipe(pd);
    pid = fork();

    if(pid)//parent as pipe WRITER
    {
        close(pd[0]);
        close(1);
        dup(pd[1]);
        close(pd[1]);
        exec(left, env);
    }
    else//child as pipe Reader
    {
        close(pd[1]);
        close(0);
        dup(pd[0]);
        close(pd[0]);
        if (pipeNum <= 0)
            exec(right, env);
        else execPipe(pipeNum, right, env);
    }
}

//turns color red
void red(void)
{
    printf("\e[38;5;196m");
}

//turns color green
void green(void)
{
    printf("\e[38;5;046m");
}

//turns color green
void blue(void)
{
    printf("\e[38;5;021m");
}

//returns text color back to white
void back(void)
{
    printf("\e[0m");
}

void fileColour(void)
{
    printf("\e[38;5;042m");
}

void dirColour(void)
{
    printf("\e[38;5;075m");
}
