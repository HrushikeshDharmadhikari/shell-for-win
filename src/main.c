/*
** A 'simple' shell program in C for Windows 8 and onward OSes.
*/

#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <direct.h>
#include <unistd.h>
#include <windows.h>
#include <sys\stat.h>
#include <time.h>

#define BUFSIZE 2
#define COUNT 64

DIR *folder, *given_folder;
char path[PATH_MAX];
int error = 0, filecount = 0;
long long int totalsize = 0;

enum error
{
    NOTHING,

    //When it cannot access the current working directory somehow
    CWD_ACCESS_ERROR,

    //When attempts to allocate memory fail
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

//function declarations in the order that they appear.
void shell_loop(char path[]);
int print_entry(unsigned short st_mode);
int builtin_count(void);
char *prepend(char *prepend, char *str, size_t size);
char *case_correct_path(char *path);
void cd(char **commands);
void help();
void exit_();
void dir(char **commands);
char *join_strings(char **strings, char *delimiters);
char *read_line(void);
char **parse_line(char *line);
int launch(char **commands);
void execute(char **commands);

//just an array of built in functions
void (*builtin_func[])(char **) =
{
    &cd,
    &help,
    &exit_,
    &dir
};

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
        perror("Directory error\a");
        _getch();
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
        folder = opendir(path);
        if(!folder)
        {
            perror("Directory error\a");
            _getch();
            exit(EXIT_FAILURE);
        }
        
        printf("\n%s>", path);
        line = read_line();
        command = parse_line(line);

        if(command)
        {
            execute(command);
        }
        
        free(line);
        free(command);

    } while (!error);

    printf("\n\n\aFATAL ERROR: ");
    printf(error_codes[error]);
    _getch();
}

//functions

int print_entry(unsigned short st_mode)
{
    if(S_ISDIR(fileStat.st_mode))
    {
        printf("%-30s", "<DIR>");
    }
    else
    {
        printf("%lld %s", fileStat.st_size, "bytes");
        printf("\t\t\t");
        totalsize += fileStat.st_size;
        filecount++;
    }
}

int builtin_count()
{
    return sizeof(builtins) / sizeof(char*);
}

char *prepend(char *prepend, char *str, size_t maxLength)
{
    int length = strlen(prepend);
    char *buffer = malloc(length + 1);

    strncpy(buffer, prepend, length);

    //contents of str are shifted by length of prepend string
    memmove(str + length, str, maxLength);
    memmove(str, prepend, maxLength);

    free(prepend);
}

char *case_correct_path(char *path)
{
    int parent_dirs_count = 50, length_of_element = PATH_MAX, i = 0;
    char **parent_dirs = (char**)malloc(parent_dirs_count * sizeof(char*)), *buffer = (char*)malloc(PATH_MAX * sizeof(char));

    HANDLE handle;
    WIN32_FIND_DATA file_data;

    if(parent_dirs == NULL)
    {
        perror("case_correct_path memory error");
        return NULL;
    }
    
    strncpy(buffer, path, PATH_MAX);
    handle = FindFirstFileA(buffer, &file_data);

    if(handle == INVALID_HANDLE_VALUE)
    {
        return NULL;
    }

    parent_dirs[i] = malloc(length_of_element * sizeof(char));
    if(parent_dirs[i] == NULL)
    {
        perror("case_correct_path memory error");
        return NULL;
    }

    strncpy(parent_dirs[i], file_data.cFileName, length_of_element);

    i++;

    while(1)
    {
        _chdir("..");
        _getcwd(buffer, PATH_MAX);

        handle = FindFirstFileA(buffer, &file_data);

        if(handle == INVALID_HANDLE_VALUE)
        {
            break;
        }

        parent_dirs[i] = malloc(length_of_element * sizeof(char));
        if(parent_dirs[i] == NULL)
        {
            perror("case_correct_path memory error");
            return NULL;
        }

        strncpy(parent_dirs[i], file_data.cFileName, length_of_element);

        i++;

        if(i >= parent_dirs_count)
        {
            parent_dirs_count += 50;
            parent_dirs = realloc(parent_dirs, parent_dirs_count);

            if(parent_dirs == NULL)
            {
                perror("case_correct_path memory error");
                return NULL;
            }
        }
    }

    parent_dirs = realloc(parent_dirs, i * sizeof(char*));

    int j = i - 1;
    while(j >= 0)
    {
        if(j != i - 1)
        {
            strncat(buffer, "\\", 2);
        }
        strncat(buffer, parent_dirs[j], length_of_element);
        j--;
    }

    j = 0;
    while(j < i)
    {
        free(parent_dirs[j]);
        j++;
    }

    free(parent_dirs);
    FindClose(handle);

    _chdir(buffer);

    return buffer;
}

void cd(char **commands)
{
    char *buffer;

    if(!commands[1])
    {
        printf("\nUsage: 'cd <folder>' to change directory. For example, 'cd test'.\n\a");
        return;
    }
    
    if(_chdir(commands[1]))
    {
        perror("Directory change error\a");
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

    buffer = case_correct_path(buffer);

    if(buffer != NULL)
    {
        strncpy(path, buffer, PATH_MAX);
    }

}

void help()
{
    FILE *help = fopen("help.txt", "r");

    if(help == NULL)
    {
        perror("Cannot get help.txt");
        return;
    }

    char buffer;

    printf("\n");

    while((buffer = fgetc(help)) != EOF)
    {
        printf("%c", buffer);
    }

    printf("\n");
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
    folder = opendir(path);
    if(folder == NULL)
    {
        error = CWD_ACCESS_ERROR;
        return;
    }

    //executes if only 'dir' was given
    if(!commands[1])
    {   
        printf("\nDirectory of %s\n\n", path);

        while(entry = readdir(folder))
        {
            //get name of entry
            stat(entry->d_name, &fileStat);
            printf("%-50s", entry->d_name);

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
        char *fullPath = malloc(PATH_MAX * sizeof(char));
        char *buffer = malloc(PATH_MAX * sizeof(char));

        strncpy(buffer, commands[1], strlen(commands[1]));

        //executes if 'dir <path>' was given

        //convert to absolute path
        if(!_fullpath(fullPath, commands[1], PATH_MAX))
        {
            perror("Absolute path error\a");
            return;
        }

        fullPath = case_correct_path(fullPath);
        if(fullPath == NULL)
        {
            perror("case_correct_path");
            strncpy(fullPath, buffer, PATH_MAX);
        }

        strncpy(buffer, commands[1], strlen(commands[1]));

        given_folder = opendir(buffer);

        if(!given_folder)
        {
            strncpy(buffer, commands[1], strlen(commands[1]));
            given_folder = opendir(buffer);
            if(!given_folder)
            {
                perror("dir\a");
                return;
            }
        }

        printf("\nDirectory of %s\n\n", fullPath);

        while(entry = readdir(given_folder))
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
    if(!joinedStr)
    {
        error = MEMORY_ERROR;
        return NULL;
    }
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
        printf("Allocation error\a");
        return NULL;
    }

    while(1)
    {
        c = getchar();

        if(c == '\n')
        {
            buffer[i] = '\0';

            buffer = strlwr(buffer);

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
                printf("Allocation error\a");
                return NULL;
            }
        }
    }
}

char **parse_line(char *line)
{
    int i = 0, j = 0, bufsize = COUNT, len, pos;
    char **tokens = malloc(bufsize * sizeof(char *));
    char *token;

    if(tokens == NULL)
    {
        perror("malloc tokens\a");
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
                perror("Allocation error\a");
                return NULL;
            }
        }
        token = strtok(NULL, "\0");
        tokens[i] = token;
        i++;
        tokens[i] = NULL;

        return tokens;
    }
    if(token != NULL && !strcmp(token, "cd."))
    {
        tokens[i] = "cd";
        i++;
        tokens[i] = ".";
        i++;
        tokens[i] = NULL;
        token = NULL;
    }
    if(token != NULL && !strcmp(token, "cd.."))
    {        
        tokens[i] = "cd";
        i++;
        tokens[i] = "..";
        i++;
        tokens[i] = NULL;
        token = NULL;
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
                perror("Allocation error\a");
                return NULL;
            }
        }
        token = strtok(NULL, " \t\r\n\a");
    }
    tokens[i] = NULL;
    return tokens;
}

int launch(char **commands)
{
    char *cmd = join_strings(commands, " ");
    

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
        if(!commands[1])
        {
            printf("Enter a valid command.\a\n");
            return 1;
        }
        printf("Error running %s (%d)\a", commands[1], GetLastError());
        return 1;
    }
}

void execute(char **commands)
{
    if(!commands[0])
    {
        return;
    }

    char *input = strlwr(commands[0]);

    for(int i = 0; i < builtin_count(); i++)
    {
        if(!strcmp(input, builtins[i]))
        {
            return (*builtin_func[i])(commands);
        }
    }

    return (void)launch(commands);
}
