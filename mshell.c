// Custom windows shell with very few features
// First real program I ever made with C though :)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <strsafe.h>
#include <windows.h>

#define READLINE_BUFSIZE 1024
#define TOKEN_BUFSIZE 64
#define TOKEN_DELIM " \t\r\n\a"


/*
 Utility function defitions
*/
char* copy_str(const char* str);
int list_dir(const char* dirpath);

/*
 Built-in shell commands:
*/
int msh_cd(char** args);
int msh_ls(char** args);
int msh_help(char** args);
int msh_exit(char** args);

// List of built-in command names
char *builtin_str[] = {
    "cd",
    "ls",
    "help",
    "exit"
};



// List of built-in command function refs
int (*builtin_func[]) (char **) = {
    &msh_cd,
    &msh_ls,
    &msh_help,
    &msh_exit
};


// Returns the number of built-in commands mshell has
int msh_num_builtins() {
    return sizeof(builtin_str) / sizeof(char *);
}


// Changes current directory
int msh_cd(char** args) {
    if (args[1] == NULL) {
        fprintf(stderr, "msh: expected argument to \"cd\"");
        return EXIT_FAILURE;
    } 

    // TBD GetCurrentDirectory shennanigans to restore previous dir on fail
    
    if (!SetCurrentDirectory(args[1])) {
        fprintf(stderr, "msh: SetCurrentDirectory failed (%lu)\n", GetLastError());
    }
    
    return EXIT_SUCCESS;
}


// Lists the contents of the current directory
int msh_ls(char** args) {
    if (list_dir((args[1] == NULL) ? "." : args[1])) {
        fprintf(stderr, "msh: ls error\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}


// Prints info on shell and builtin commands
int msh_help(char** args) {
    printf("");

    for (int i = 0; i < msh_num_builtins(); i++) {
        printf(" %s\n", builtin_str[i]);
    }
    
    printf("Use the man command for information on other programs.\n");
    
    return EXIT_SUCCESS;
}


// Exits the shell
int msh_exit(char **args) {
    exit(EXIT_SUCCESS);
}


// Reads next user-input line
char* msh_read_line() {
    int bufsize = READLINE_BUFSIZE;
    char* buffer = malloc(sizeof(char) * bufsize);
    
    if (!buffer) {
        fprintf(stderr, "msh: allocation error\n");
        exit(EXIT_FAILURE);
    }

    int position = 0;
    int c;
    
    while (1) {
        // Read a character
        c = getchar();

        // EOF and newlines end the read--placing
        // null character at current position
        if (c == EOF || c == '\n') {
            buffer[position] = '\0';
            return buffer;
        }

        buffer[position] = c;
        position++;

        // If buffer has been exceeded, then memory is reallocated
        if (position >= bufsize) {
            bufsize += READLINE_BUFSIZE;
            buffer = realloc(buffer, bufsize * sizeof(char));
            if (!buffer) {
                fprintf(stderr, "msh: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}


// Splits a string into tokens based on whitespace and nothing else
char** msh_split_line(const char* line) {
    int bufsize = TOKEN_BUFSIZE;    
    char** tokens = malloc(bufsize * sizeof(char*));
    if (!tokens) {
        fprintf(stderr, "msh: allocation error");
        exit(EXIT_FAILURE);
    }
    
    char *temp = copy_str(line);
    char *context = "";    
    int position = 0;    
    char * token = strtok_s(temp, TOKEN_DELIM, &context);

    while (token) {

        if (position >= bufsize) {
            bufsize += TOKEN_BUFSIZE;
            tokens = realloc(tokens, bufsize * sizeof(char*));
            if (!tokens) {
                fprintf(stderr, "msh: allocation error");
                exit(EXIT_FAILURE);
            }
        }

        tokens[position] = token;
        position++;

        token = strtok_s(NULL, TOKEN_DELIM, &context);
    }

    // Adding null terminator
    tokens[position] = NULL;
    
    return tokens;
}


// Attempts to launch a process based on the given command line
int msh_launch(char* line) {
    STARTUPINFO si = { sizeof(si) };
    PROCESS_INFORMATION pi;

    // Allocates and clears memory
    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);
    ZeroMemory( &pi, sizeof(pi) );
    
    // Start the child process
    if (
        !CreateProcess( 
            NULL,     // Application name
            line,         // Command line to run
            NULL,   // Handle cannot be inherited       
            NULL,    // Security descriptor
            FALSE,      // Sets whether inheritable handles are passed to process
            0,          // Flags controlling priority class and the creation of process.
            NULL,         // New process uses environment block of calling process
            NULL,    // Uses same directory as calling process 
            &si,          
            &pi
        )
    ) {
        fprintf(stderr, "CreateProcess failed (%lu).\n", GetLastError());
        // return EXIT_FAILURE;
    }
    
    // Wait until child process exits
    WaitForSingleObject(pi.hProcess, INFINITE);

    // Close process and thread handles
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    
    return EXIT_SUCCESS;
}


int msh_execute(char **args, char* line) {
    int i;

    if (args[0] == NULL) {
        return 0; 
    }
    
    for (i = 0; i < msh_num_builtins(); i++) {
        if (strcmp(args[0], builtin_str[i]) == 0) {
            return (*builtin_func[i])(args);
        }
    }
    
    return msh_launch(line);
}


// Main shell loop
void msh_loop(void) {
    char* line = "";
    char** args;
    int status = 0;

    do {
        
        printf("\n>");
        line = msh_read_line();

        // printf("Line received: %s\n", line);
        
        args = msh_split_line(line);
        // printf("Args: ");

        // for (int i = 0; i < sizeof(args) / sizeof(args[0]); i++) {
        //     printf("%s size %zu", args[i], strlen(args[i]));
        // }
        // printf("\n");

        // printf("Line post-tokenize: %s\n", line);
        
        status = msh_execute(args, line);
        // printf("\nExit status: %d", status);
        
        free(line);
        free(args);
        
    } while(!status);
}


// Returns a copy of a string of variable size
char* copy_str(const char* str) {
    // + 1 for null terminator
    size_t bufsize = strlen(str) + 1;

    char* out = malloc(bufsize);
    if (!out) {
        fprintf(stderr, "msh: allocation error");
        exit(EXIT_FAILURE);
    }

    strcpy_s(out, bufsize, str);

    return out;
}


// Lists the contents of the given directory
int list_dir(const char*dirpath) {
    WIN32_FIND_DATA ffd;
    LARGE_INTEGER filesize;
    TCHAR sz_dir[MAX_PATH];
    size_t path_length;
    HANDLE h_find = INVALID_HANDLE_VALUE;
    DWORD dw_err = 0;
    
    // Checking that input path doesn't exceed 
    // MAX_PATH + 3 for \* plus NULL appended 
    StringCchLength(dirpath, MAX_PATH, &path_length);
    
    if ( path_length > (MAX_PATH - 3) ) {
        fprintf(stderr, "msh: Directory path is too long.\n");
        return EXIT_FAILURE;
    }

    // Prepare string for FindFile functions.
    // First, copy to buffer, then append '\*' to the directory name.
    StringCchCopy(sz_dir, MAX_PATH, dirpath);
    StringCchCat(sz_dir, MAX_PATH, TEXT("\\*"));

    // Find the first file in the directory
    h_find = FindFirstFile(sz_dir, &ffd);

    if (h_find == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "msh: error finding first file\n");
        printf("Attempted dirpath: %s\n", dirpath);
        return EXIT_FAILURE;
    }

    // List all files in the directory with some additional info
    do {
        // Skips special dirs "." and ".." which windows always includes
        if (strcmp(ffd.cFileName, ".") == 0 || strcmp(ffd.cFileName, "..") == 0) {
            continue;
        }
        
        // Directories
        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            _tprintf(TEXT("    %s\t\t<DIR>\n"), ffd.cFileName);

        // Files
        } else {
            filesize.LowPart = ffd.nFileSizeLow;
            filesize.HighPart = ffd.nFileSizeHigh;
            _tprintf(TEXT("    %s\t\t%lld bytes\n"), ffd.cFileName, filesize.QuadPart);
        }
        
    } while (FindNextFile(h_find, &ffd) != 0);

    dw_err = GetLastError();
    if (dw_err != ERROR_NO_MORE_FILES) {
        fprintf(stderr, "msh: ls error");
    }

    FindClose(h_find);
    return EXIT_SUCCESS;
}


int main (int argc, char* argv[]) {

    // Run command loop
    msh_loop();

    return EXIT_SUCCESS;
}
