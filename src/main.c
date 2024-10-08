/*
* A 'simple' shell program in C for Windows 8 and onward oses.
*/


#define BUFSIZE 4096
#define COUNT 64
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <direct.h>
#include <unistd.h>
#include <windows.h>
#include <sys\stat.h>
#include <time.h>

DIR *folder, *givenFolder;
char path[PATH_MAX];
int error = 0, filecount = 0;
long long int totalsize = 0;

enum error
{
    //the names NO_ERROR and NOERROR were defined already, so had to use this.
    NOTHING,
    CWD_ACCESS_ERROR,
    MEMORY_ERROR
};

char *error_codes[] = 
{
    "No error",
    "Current directory error",
    "Memory allocation error"
};

//for getting folder info
struct dirent *entry;
struct stat fileStat; 

//built in command list
char *builtins[] =
{
    "cd",
    "help",
    "exit",
    "dir"
};

//built in commands function definitions
void cd(char **commands);
void help();
void exit_();
void dir(char **commands);

//just an array of built in functions
void (*builtin_func[])(char **) =
{
    &cd,
    &help,
    &exit_,
    &dir
};

//function declarations in the order that they appear.
int print_entry(unsigned short st_mode);
int builtin_count();
char *join_strings(char **strings, char *delimiters);
char *read_line();
char **parse_line(char *line);
int launch(char **command);
void execute(char **commands);
void shell_loop(char path[]);

int main()
{
    printf("Shell starting...\n\n");

    if(!_getcwd(path, PATH_MAX))
    {
        perror("getcwd error \a");
        exit(EXIT_FAILURE);
    }
    folder = opendir(path);

    if(!folder)
    {
        perror("Directory error");
        getchar();
        exit(EXIT_FAILURE);
    }

    shell_loop(path);

    return EXIT_FAILURE;
}

void shell_loop(char path[])
{
    char *line;
    char **command;
    
    do
    {
        //capitalise drive letter
        if(path[0] >= 97 && path[0] <= 122)
        {
            path[0] -= 32;
        }

        //open folder for commands like dir
        
        printf("\n%s>", path);
        line = read_line(line);
        command = parse_line(line);

        if(command)
        {
            execute(command);
        }
        
        free(line);
        free(command);

    } while (!error);

    printf("\n\nFATAL ERROR: ");
    printf(error_codes[error]);
    getchar();
}

//functions

int print_entry(unsigned short st_mode)
{
    if(S_ISDIR(fileStat.st_mode))
    {
        printf("%-12s", "<DIR>");
    }
    else
    {
        printf("%-12lld", fileStat.st_size);
        totalsize += fileStat.st_size;
        filecount++;
    }
}

int builtin_count()
{
    return sizeof(builtins) / sizeof(char*);
}

void cd(char **commands)
{
    char *buffer;

    if(!commands[1])
    {
        printf("\nUsage: 'cd <folder>' to change directory. For example, 'cd test'.\n");
        return;
    }
    if(_chdir(commands[1]))
    {
        perror("Directory change error");
        return;
    }

    if(!(buffer = _getcwd(NULL, 0)))
    {
        perror("getcwd error \a");

        //path is still unchanged, so revert directory to original.

        if(_chdir(path))
        {
            error = CWD_ACCESS_ERROR;
        }
        return;
    }
    strncpy(path, buffer, PATH_MAX);
}

void help()
{
    printf("\n%-30s :- Changes directory to specified directory (cd.. for going to parent directory).", "cd <directory-name>");
    printf("\n%-30s :- Shows what is in the current directory", "dir");
    printf("\n%-30s :- Shows what is in the specified directory", "dir <directory-name>");
    printf("\n%-30s :- Exits the shell.\n", "exit");

    printf("\nGo to GitHub for source code (Hrushikesh Dharmadhikari).\n");
}

void exit_()
{
    if(folder)
    {
        closedir(folder);
    }
    
    exit(EXIT_SUCCESS);
}

void dir(char **commands)
{
    char tempPath[PATH_MAX];

    //temppath is used to obtain absolute path from relative path.

    //executes if only 'dir' was given
    if(!commands[1])
    {
        printf("\nDirectory of %s\n\n", path);

        while(entry = readdir(folder))
        {
            //get name of entry
            stat(entry->d_name, &fileStat);
            printf("%-30s", entry->d_name);

            print_entry(fileStat.st_mode);

            //prints date

            printf("%s", ctime(&fileStat.st_mtime));
        }
        closedir(folder);

        printf("\n%d Files for %lld bytes", filecount, totalsize);
        filecount = 0;
        totalsize = 0;
    }
    else
    {
        //executes if 'dir <path>' was given

        //convert to absolute path
        if(!_fullpath(tempPath, commands[1], PATH_MAX))
        {
            perror("Absolute path error");
            return;
        }

        givenFolder = opendir(tempPath);

        if(!givenFolder)
        {
            strncpy(tempPath, commands[1], PATH_MAX);
            givenFolder = opendir(tempPath);
            if(!givenFolder)
            {
                perror("dir");
                return;
            }
        }

        printf("\nDirectory of %s\n\n", tempPath);

        while(entry = readdir(givenFolder))
        {
            stat(entry->d_name, &fileStat);
            printf("%s\n", entry->d_name);
            filecount++;
        }
        closedir(folder);

        printf("\n%d Files", filecount);
        filecount = 0;
    }
}

char *join_strings(char **strings, char *delimiters)
{
    char* joinedStr;
    int i = 1;

    joinedStr = realloc(NULL, strlen(strings[0]) + 1);
    strcpy(joinedStr, strings[0]);

    if (strings[0] == NULL) {
        return joinedStr;
    }

    while (strings[i] != NULL) {
        joinedStr = (char*)realloc(joinedStr, strlen(joinedStr) + strlen(strings[i]) + strlen(delimiters) + 1);
        strcat(joinedStr, delimiters);
        strcat(joinedStr, strings[i]);
        i++;
    }

    return joinedStr;
}

char *read_line()
{
    char c;
    int buf_size = BUFSIZE, i = 0;

    char *buffer = malloc(buf_size);

    if(!buffer)
    {
        printf("Allocation error");
        return NULL;
    }

    while(1)
    {
        c = getchar();

        if(c == '\n')
        {
            buffer[i] = '\0';
            return buffer;
            break;
        }
        else
        {
            buffer[i] = c;
        }
        i++;

        if(i >= buf_size)
        {
            buf_size += BUFSIZE;
            buffer = realloc(buffer, buf_size);
            if(!buffer)
            {
                printf("Allocation error");
                return NULL;
            }
        }
    }
}

char **parse_line(char *line)
{
    //

    int i = 0, j = 0, bufsize = COUNT, len, pos;
    char **tokens = malloc(bufsize * sizeof(char *));
    char *token;

    if(tokens == NULL)
    {
        perror("malloc tokens");
        error = MEMORY_ERROR;
        return NULL;
    }

    token = strtok(line, " \t\r\n\a");

    if(!token)
    {
        return NULL;
    }

    if(!strcmp(token, "cd") || !strcmp(token, "start"))
    {
        tokens[i] = token;
        i++;
        if(i >= bufsize)
        {
            bufsize += COUNT;
            tokens = realloc(tokens, bufsize * sizeof(char*));
            if(!tokens)
            {
                perror("Allocation error");
                return NULL;
            }
        }
        token = strtok(NULL, "\0");
        tokens[i] = token;
        i++;
        tokens[i] = NULL;
        return tokens;
    }
    if(!strcmp(token, "cd."))
    {
        tokens[i] = "cd";
        i++;
        tokens[i] = ".";
        i++;
        tokens[i] = NULL;
    }
    if(!strcmp(token, "cd.."))
    {        
        tokens[i] = "cd";
        i++;
        tokens[i] = "..";
        i++;
        tokens[i] = NULL;
    }

    while(token != NULL)
    {
        tokens[i] = token;
        i++;

        if(i >= bufsize)
        {
            bufsize += COUNT;
            tokens = realloc(tokens, bufsize * sizeof(char*));
            if(!tokens)
            {
                perror("Allocation error");
                return NULL;
            }
        }
        token = strtok(NULL, " \t\r\n\a");
    }
    tokens[i] = NULL;
    return tokens;
}

int launch(char **command)
{
    char *cmd = join_strings(command, " ");
    

    STARTUPINFO startupInfo = 
    {
        sizeof(startupInfo)
    };
    PROCESS_INFORMATION processInfo;

    if(CreateProcess(NULL, cmd, NULL, NULL, 1, 0 ,NULL, NULL, &startupInfo, &processInfo))
    {
        //wait for process to end
        WaitForSingleObject(processInfo.hProcess, INFINITE);

        //clean up
        CloseHandle(processInfo.hProcess);
        CloseHandle(processInfo.hThread);
    } 
    else
    {
        if(!command[1])
        {
            printf("Enter a valid command.");
            return 1;
        }
        printf("Error running %s (%d)", command[1], GetLastError());
        return 1;
    }
}

void execute(char **commands)
{
    if(!commands[0])
    {
        return;
    }

    for(int i = 0; i < builtin_count(); i++)
    {
        if(!strcmp(commands[0], builtins[i]))
        {
            return (*builtin_func[i])(commands);
        }
    }

    return (void)launch(commands);
}
