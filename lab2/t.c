#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//struct for all nodes in file tree
typedef struct node
{
    char name[64], type;
    struct Node *parentPtr, *childPtr, *siblingPtr;
}Node;

Node *root, *cwd, *cur;             // root and CWD pointers
char line[128];                     // user input command line
char command[16], pathname[64];     // command and pathname strings
char path2[64];
char dname[64], bname[64];          // dirname and basename string holders
int file;
//array for comand names which will retreave pointer index
char *cmd[] = { "mkdir", "rmdir", "cd", "ls", "pwd", "creat", "rm", "save",
                "reload", "menu", "quit", NULL};
char *dirNames[64] = { NULL };
int cmdIndex, dirIndex;

void initDir(void);
void directions(void);
Node* newNode(char *nName, char nType);
void getUserInput(void);
int getCmdIndex(char *nCmd);
void printCurCwd(void);

//all command functions
int mkdir(char *pathName);
int rmdir(char *pathName);
int cd(char *pathName);
int ls(char *pathName);
int pwd(char *pathName);
int creat(char *pathName);
int rm(char *pathName);
int save(char *pathName);
int reload(char *pathName);
int menu(char *pathName);
int quit(char *pathName);

//color functions
void red(void);
void green(void);
void blue();
void back(void);
void fileColour(void);
void dirColour(void);

int tokenize(char *pathname);
int findDir(int nIndex, char nType);
int createDirFile(char nType);
int removeDirFile(char nType);
int lsPrint(void);
int printTreeToFile(char *fileName);
void printTree(FILE *pf, Node *cur);
void getPath(Node *path);
void getPathHelper(Node *path);
int getFile(char *fileName);

int(*fptr[])(char *) = { (int(*)())mkdir,rmdir,cd,ls,pwd,creat,rm,save,
                        reload,menu,quit };

int main(void)
{
    int cmdS = 0;   //integer that will hold if cmd was sucessfull or not
    cmdIndex = dirIndex = -1;
    file = 0;
    initDir();
    directions();

    while(cmdIndex != 10)
    {
        printCurCwd();
        getUserInput();
        printf("\n\nCommand inputed: %s\t%s\n", command, pathname);
        cmdIndex = getCmdIndex(command);
        if(cmdIndex != -1)
        {
            cmdS = fptr[cmdIndex](pathname);
            if (!cmdS)
            {
                red();
                printf("Command can't be Executed, type \"menu\" for available commands\n");
            }
            else
            {
                green();
                printf("Valid Command Executed\n");
            }
            back();
        }
        printf("\n\n");
        command[0] = pathname[0] = '\0';
    }
    return 0;
}

//first initial directory initiator
void initDir(void)
{
    root = cwd = cur = newNode("/", 'D');
}

//small set of directions for program
void directions(void)
{
    red();
    printf("Welcome to THE file system\n\n");
    back();
    printf("**********************************************************************\n");
    printf("Available comands:\n");
    printf("mkdir, rmdir, cd, ls, pwd, creat, rm, save, reload, menu, quit\n");
    printf("**********************************************************************\n");
}

//creates new node with data provided
Node* newNode(char *nName, char nType)
{
    Node *nNode = (Node *)malloc(sizeof(Node));
    strcpy(nNode->name, nName);
    nNode->type = nType;
    nNode->parentPtr = NULL;
    nNode->childPtr = NULL;
    nNode->siblingPtr = NULL;
    return nNode;
}

//function that gets users input using sscanf()
void getUserInput(void)
{
    fgets(line, 128, stdin); //retrieves 128 at max
    line[strlen(line) - 1] = 0; //removes \n at end of the line
    sscanf(line, "%s %s", command, pathname);
}

//finds commands index in array, if not found returns one and prints
//about the command
int getCmdIndex(char *nCmd)
{
    for (int i = 0; cmd[i]; i++)
    {
        if (!strcmp(cmd[i], nCmd)) return i;
    }
    red();
    printf("No Such Command Exists in Current Program\n");
    back();
    return -1;
}


void printCurCwd(void)
{
    getPath(cwd);
    printf("\e[38;5;046m");
    printf("user@KS-Virtual");
    back();
    printf(":");
    printf("\e[38;5;075m");
    if (strcmp(cwd->name, "\\") == 0) printf("~");
    else printf("~%s", path2);
    back();
    printf("$ ");
}

int mkdir(char *pathName)
{
    if (!file) printf("*****\tmkdir\t*****\n");
    int i = 0;
    if (!strcmp(pathName, "")) return 0;
    tokenize(pathName);
    if (pathName[0] == '/')
    {
        cur = root;
        i = findDir(0, 'D');
        if (i) i = createDirFile('D');
        cur = cwd;
    }
    else
    {
        cur = cwd;
        i = findDir(0, 'D');
        if (i) i = createDirFile('D');
        cur = cwd;
    }
    return i;
}

int rmdir(char *pathName)
{
    printf("*****\trmdir\t*****\n");
    int i = 0;
    if (!strcmp(pathName, "")) return 0;
    tokenize(pathName);
    if (pathName[0] == '/')
    {
        cur = root;
        i = findDir(0, 'D');
        if (i) i = removeDirFile('D');
        cur = cwd;
    }
    else
    {
        cur = cwd;
        i = findDir(0, 'D');
        if (i) i = removeDirFile('D');
        cur = cwd;
    }
    return i;
}

int cd(char *pathName)
{
    if (!file) printf("*****\tcd\t*****\n");
    int i = 0;
    if (!strcmp(pathName, ""))
    {
        cwd = root;
    }
    else
    {
        cur = cwd;
        tokenize(pathName);
        dirIndex += 1;
        int x = 0;
        for (; x < dirIndex && strcmp(dirNames[x], "..") == 0; x++)
        {
            if (cur->parentPtr != NULL)
            {
                cur = cur->parentPtr;
            }
            else
            {
                cur = cwd;
                red();
                printf("Impossible to complete, cant go higher then \"/\"\n");
                back();
                return 0;
            }
        }
        if (x < 0) x = 0;
        if (x < dirIndex) i = findDir(x, 'D');
        else
        {
            cwd = cur;
            return 1;
        }
        if (i) cwd = cur;
        else return 0;
    }
    return 1;
}

int ls(char *pathName)
{
    printf("*****\tls\t*****\n");
    int i = 0;
    if (!strcmp(pathName, ""))
    {
        cur = cwd;
        lsPrint();
        cur = cwd;
    }
    else
    {
        cur = cwd;
        tokenize(pathName);
        dirIndex++;
        i = findDir(0, 'D');
        if (i) lsPrint();
        else return 0;
        cur = cwd;
    }
    return 1;
}

int pwd(char *pathName)
{
    printf("*****\tpwd\t*****\n");
    if (strcmp(pathName, "") != 0) return 0;
    getPath(cwd);
    printf("%s\n", path2);
    return 1;
}

int creat(char *pathName)
{
    if(!file) printf("*****\tcreate\t*****\n");
    int i = 0;
    if (!strcmp(pathName, "")) return 0;
    tokenize(pathName);
    if (pathName[0] == '/')
    {
        cur = root;
        i = findDir(0, 'F');
        if (i) i = createDirFile('F');
        cur = cwd;
    }
    else
    {
        cur = cwd;
        i = findDir(0, 'F');
        if (i) i = createDirFile('F');
        cur = cwd;
    }
    return i;
}

int rm(char *pathName)
{
    printf("*****\trm\t*****\n");
    int i = 0;
    if (!strcmp(pathName, "")) return 0;
    tokenize(pathName);
    if (pathName[0] == '/')
    {
        cur = root;
        i = findDir(0, 'F');
        if (i) i = removeDirFile('F');
        cur = cwd;
    }
    else
    {
        cur = cwd;
        i = findDir(0, 'F');
        if (i) i = removeDirFile('F');
        cur = cwd;
    }
    return i;
}

int save(char *fileName)
{
    printf("*****\tsave\t*****\n");
    if (strcmp(fileName, "") == 0) return 0;
    printTreeToFile(fileName);
    return 1;
}

int reload(char *fileName)
{
    int i = 0;
    printf("*****\treload\t*****\n");
    if (strcmp(fileName, "") == 0) return 0;
    file = 1;
    i = getFile(fileName);
    file = 0;
    return i;
}

//simple function that shows available commands
int menu(char *pathName)
{
    printf("*****\tmenu\t*****\n");
    if (!strcmp(pathName, ""))
    {
        printf("**********************************************************************\n");
        printf("Available comands:\n");
        printf("mkdir, rmdir, cd, ls, pwd, creat, rm, save, reload, menu, quit\n");
        printf("**********************************************************************\n");
        return 1;
    }
    return 0;
}

//this function quits the program
int quit(char *pathName)
{
    printf("*****\tquit\t*****\n");
    if (!strcmp(pathName, ""))
    {
        if(save("myfile")) return 1;
    }
    return 0;
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

//tokenizer function
int tokenize(char *pathname)
{
    for (int i = 0; i < 64; i++)    //sets all values in array to null so a new path can be interpreted
    {
        dirNames[i] = NULL;
    }

    dirNames[0] = strtok(pathname, "/"); // first call to strtok()
    int i = 1;
    for (; dirNames[i - 1]; i++)
    {
        dirNames[i] = strtok(0, "/"); // calling strtok() until it returns NULL
    }
    dirIndex = i - 2;
}

//finds directory specified by the path
int findDir(int nIndex, char nType)
{
    //printf("%s\t%d\n", cur->name, nIndex);
    if (nIndex < dirIndex)//checks if at name of new file or directory
    {
        cur = cur->childPtr;//goes to child so into directory
                            //to look for name
        while (cur != NULL) //while it hasnt reached end of directory 
        {
            //printf("%s\n", cur->name);
            if (strcmp(dirNames[nIndex], cur->name) == 0 && cur->type == 'D')
            {
                return findDir(++nIndex, nType);
            }
            cur = cur->siblingPtr;
        }
    }
    else
    {
        if (!file)
        {
            green();
            printf("Directory found\n");
            back();
        }
        return 1;
    }
    if (!file)
    {
        red();
        printf("Directory not found\n");
        back();
    }
    return 0;
}

//creates the file or dir in the location specified if no duplicates exist
int createDirFile(char nType)
{
    Node *parent = cur;
    if (cur->childPtr == NULL) //checks if directory empty
    {
        cur->childPtr = newNode(dirNames[dirIndex], nType);
        Node *temp = cur->childPtr;
        temp->parentPtr = parent;
    }
    else //if not finds the ends
    {
        Node *temp = cur;
        temp = cur->childPtr;
        while (temp != NULL)//goes through all names in director
        {
            //if any match by name and file type doesnt allow to makes dir of file
            if (strcmp(temp->name, dirNames[dirIndex]) == 0 && temp->type == nType)
            {
                red();
                if (nType == 'F') printf("File with %s name already exists\n", dirNames[dirIndex]);
                else printf("Directory with %s name already exists\n", dirNames[dirIndex]);
                back();
                return 0;
            }
            cur = temp;
            temp = temp->siblingPtr;
        }
        cur->siblingPtr = newNode(dirNames[dirIndex], nType);
        cur = cur->siblingPtr;
        cur->parentPtr = parent;
    }
    if (!file)
    {
        green();
        if (nType == 'F') printf("File with %s name was created\n", dirNames[dirIndex]);
        else printf("Directory with %s name was created\n", dirNames[dirIndex]);
        back();
    }
    return 1;
}

int removeDirFile(char nType)
{
    Node *temp = cur->childPtr;
    if (strcmp(cur->childPtr, dirNames[dirIndex]) == 0 && temp->type == nType) //checks if child is the directory or file to delete
    {   
        if (temp->childPtr == NULL)
        {
            cur->childPtr = temp->siblingPtr;
            free(temp);
            if (nType == 'D')
            {
                green();
                printf("Directory %s has been deleted\n", dirNames[dirIndex]);
                back();
            }
            else
            {
                green();
                printf("File %s has been deleted\n", dirNames[dirIndex]);
                back();
            }
            return 1;
        }
        red();
        printf("Directory %s has children can not be delete!!!\n", dirNames[dirIndex]);
        back();
    }
    else //if not finds the file or dir
    {
        temp = cur->childPtr;
        while (temp != NULL)//goes through all names in director
        {
            if (strcmp(temp->name, dirNames[dirIndex]) == 0 && temp->type == nType)
            {
                if (temp->childPtr == NULL)
                {
                    cur->siblingPtr = temp->siblingPtr;
                    free(temp);
                    if (nType == 'D')
                    {
                        green();
                        printf("Directory %s has been deleted\n", dirNames[dirIndex]);
                        back();
                    }
                    else
                    {
                        green();
                        printf("File %s has been deleted\n", dirNames[dirIndex]);
                        back();
                    }

                    return 1;
                }
                red();
                printf("Directory %s has children can not be delete!!!\n", dirNames[dirIndex]);
                back();
                return 0;
            }
            cur = temp;
            temp = temp->siblingPtr;
        }
        if (nType == 'D')
        {
            red();
            printf("Directory %s does not exist\n", dirNames[dirIndex]);
            back();
        }
        else
        {
            red();
            printf("File %s does not exist\n", dirNames[dirIndex]);
            back();
        }
    }
    return 0;
}

int lsPrint(void)
{
    if (cur->childPtr == NULL) //checks if directory empty
    {
        printf("The directory is empty, nothing to print\n");
    }
    else //if not prints
    {
        cur = cur->childPtr;
        while (cur != NULL)//goes through all names in director
        {
            if (cur->type == 'F') fileColour();
            else dirColour();
            printf("%c %s\n", cur->type, cur->name);
            back();
            cur = cur->siblingPtr;
        }
    }
    return 1;
}

//prints to file, there were a lot of problems with end of file and nothing worked so i changed it to ..... .....
int printTreeToFile(char *fileName)
{
    FILE *fp = fopen(fileName, "w+");
    if (fp != NULL)
    {
        printTree(fp, root);
        fprintf(fp, "..... .....");
        fclose(fp);
        return 1;
    }
    return 0;
}

//prints the tree to file
void printTree(FILE *pf, Node *cur)
{
    if (cur == NULL) return;
    path2[0] = '\0';
    getPath(cur);
    fprintf(pf, "%c\t%s\n", cur->type ,path2);
    printTree(pf, cur->childPtr);
    printTree(pf, cur->siblingPtr);
}

//gets path and resets old on
void getPath(Node *path)
{
    path2[0] = '\0';
    getPathHelper(path);
}

//helps getting the current path
void getPathHelper(Node *path)
{
    if (path == NULL)
    {
        return;
    }
    if (path->parentPtr != NULL)
    {
        Node *temp = path->parentPtr;
        getPathHelper(temp);
        if (strcmp(temp->name, "/") != 0) strcat(path2, "/");
        strcat(path2, path->name);
    }
    else
    {
        strcat(path2, "/");
    }
    return;
}

//function that gets users input using sscanf()
int getFile(char *fileName)
{
    FILE *fp = fopen(fileName, "r");
    if (fp != NULL)
    {
        fgets(line, 128, fp); //retrieves 128 at max
        line[strlen(line) - 1] = 0; //removes \n at end of the line
        sscanf(line, "%s %s", command, pathname);
        if ((strcmp(command, "D") == 0) && (strcmp(pathname, "/") == 0))
        {
            fgets(line, 128, fp);
            while (strcmp(line, "..... .....") != 0)
            {
                line[strlen(line) - 1] = 0;
                sscanf(line, "%s %s", command, pathname);
                if (strcmp(command, "D") == 0) mkdir(pathname);
                else if ((strcmp(command, "F") == 0)) creat(pathname);
                fgets(line, 128, fp);
            }
        }
        else 
        {
            printf("improper file format\n");
            return 0;
        }
        fclose(fp);
        return 1;
    }
    else printf("Error Readin file!\n");
    return 0;
}