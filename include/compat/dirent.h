/*
 * Dirent interface for Windows
 * Based on Toni Ronkko's dirent.h for Windows
 * Public domain; no warranty
 */
#ifndef DIRENT_H
#define DIRENT_H

#if defined(_WIN32) || defined(_WIN64)

#include <windows.h>
#include <string.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Maximum length of file name */
#define NAME_MAX 260

/* File types */
#define DT_UNKNOWN 0
#define DT_REG 8
#define DT_DIR 4

/* Directory entry structure */
struct dirent {
    long d_ino;              /* Inode number (not used on Windows) */
    unsigned char d_type;    /* File type */
    char d_name[NAME_MAX];   /* File name */
};

/* Directory stream structure */
typedef struct DIR {
    struct dirent ent;
    HANDLE handle;
    WIN32_FIND_DATAA data;
    int cached;
} DIR;

/* Open directory stream */
static DIR* opendir(const char* dirname) {
    DIR* dir = NULL;
    char path[MAX_PATH];
    
    if (dirname == NULL || dirname[0] == '\0') {
        errno = ENOENT;
        return NULL;
    }
    
    /* Construct search pattern */
    size_t len = strlen(dirname);
    if (len + 3 > MAX_PATH) {
        errno = ENAMETOOLONG;
        return NULL;
    }
    
    strcpy(path, dirname);
    if (path[len - 1] != '/' && path[len - 1] != '\\') {
        path[len++] = '\\';
    }
    path[len++] = '*';
    path[len] = '\0';
    
    /* Allocate directory structure */
    dir = (DIR*)malloc(sizeof(DIR));
    if (dir == NULL) {
        errno = ENOMEM;
        return NULL;
    }
    
    /* Open directory */
    dir->handle = FindFirstFileA(path, &dir->data);
    if (dir->handle == INVALID_HANDLE_VALUE) {
        free(dir);
        errno = ENOENT;
        return NULL;
    }
    
    dir->cached = 1;
    return dir;
}

/* Read directory entry */
static struct dirent* readdir(DIR* dir) {
    if (dir == NULL || dir->handle == INVALID_HANDLE_VALUE) {
        errno = EBADF;
        return NULL;
    }
    
    /* Use cached entry or read next */
    if (dir->cached) {
        dir->cached = 0;
    } else {
        if (!FindNextFileA(dir->handle, &dir->data)) {
            return NULL;
        }
    }
    
    /* Copy file name */
    strncpy(dir->ent.d_name, dir->data.cFileName, NAME_MAX - 1);
    dir->ent.d_name[NAME_MAX - 1] = '\0';
    
    /* Set file type */
    if (dir->data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        dir->ent.d_type = DT_DIR;
    } else {
        dir->ent.d_type = DT_REG;
    }
    
    dir->ent.d_ino = 0;
    
    return &dir->ent;
}

/* Close directory stream */
static int closedir(DIR* dir) {
    if (dir == NULL || dir->handle == INVALID_HANDLE_VALUE) {
        errno = EBADF;
        return -1;
    }
    
    FindClose(dir->handle);
    free(dir);
    return 0;
}

#ifdef __cplusplus
}
#endif

#else
/* Use system dirent.h on POSIX systems */
#include <dirent.h>
#endif

#endif /* DIRENT_H */
