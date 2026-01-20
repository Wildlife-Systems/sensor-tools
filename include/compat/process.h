/*
 * Cross-platform compatibility header for sensor-mon
 * Provides abstraction for POSIX/Windows differences
 */
#ifndef COMPAT_PROCESS_H
#define COMPAT_PROCESS_H

#ifdef _WIN32
    /* Windows compatibility */
    #include <windows.h>
    #include <process.h>
    #include <io.h>
    #include <direct.h>
    
    /* Use PDCurses on Windows */
    #include <curses.h>
    
    /* Windows doesn't have these POSIX headers */
    #define access _access
    #define X_OK 0  /* Windows doesn't have X_OK, just check existence */
    #define F_OK 0
    
    /* Pipe handling */
    #define popen _popen
    #define pclose _pclose
    
    /* Path separator */
    #define PATH_SEPARATOR ';'
    #define DIR_SEPARATOR '\\'
    
#else
    /* POSIX systems (Linux, macOS) */
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/wait.h>
    #include <dirent.h>
    #include <ncurses.h>
    
    /* Path separator */
    #define PATH_SEPARATOR ':'
    #define DIR_SEPARATOR '/'
    
#endif

/* Common includes */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <sys/stat.h>

/*
 * Cross-platform function to run a command and capture its output.
 * Returns malloc'd string with output, or NULL on error.
 * Caller must free the returned string.
 */
static inline char *run_command_capture(const char *command) {
    FILE *fp = popen(command, "r");
    if (!fp) return NULL;
    
    size_t buf_size = 4096;
    char *output = (char*)malloc(buf_size);
    if (!output) {
        pclose(fp);
        return NULL;
    }
    
    size_t total = 0;
    size_t bytes_read;
    char temp[256];
    
    while ((bytes_read = fread(temp, 1, sizeof(temp) - 1, fp)) > 0) {
        if (total + bytes_read >= buf_size - 1) {
            buf_size *= 2;
            char *new_buf = (char*)realloc(output, buf_size);
            if (!new_buf) {
                free(output);
                pclose(fp);
                return NULL;
            }
            output = new_buf;
        }
        memcpy(output + total, temp, bytes_read);
        total += bytes_read;
    }
    output[total] = '\0';
    
    pclose(fp);
    return output;
}

/*
 * Cross-platform function to run a command and get exit status.
 * Returns the exit code, or -1 on error.
 */
static inline int run_command_status(const char *command) {
#ifdef _WIN32
    return system(command);
#else
    int status = system(command);
    if (status == -1) return -1;
    if (WIFEXITED(status)) return WEXITSTATUS(status);
    return -1;
#endif
}

/*
 * Check if a file is executable.
 * On Windows, checks for .exe extension or just existence.
 * On POSIX, uses access() with X_OK.
 */
static inline int is_executable(const char *path) {
#ifdef _WIN32
    /* On Windows, check for common executable extensions or just existence */
    if (access(path, F_OK) != 0) return 0;
    
    const char *ext = strrchr(path, '.');
    if (ext) {
        if (_stricmp(ext, ".exe") == 0 || _stricmp(ext, ".bat") == 0 ||
            _stricmp(ext, ".cmd") == 0 || _stricmp(ext, ".com") == 0) {
            return 1;
        }
    }
    /* Also check without extension for compatibility */
    return 1;
#else
    return access(path, X_OK) == 0;
#endif
}

#endif /* COMPAT_PROCESS_H */
