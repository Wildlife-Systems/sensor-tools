/*
 * Cross-platform unistd compatibility header
 */
#ifndef COMPAT_UNISTD_H
#define COMPAT_UNISTD_H

#if defined(_WIN32) || defined(_WIN64)

#include <io.h>
#include <direct.h>
#include <process.h>

/* Windows equivalents for POSIX functions */
#ifndef mkdir
#define mkdir(path, mode) _mkdir(path)
#endif
#ifndef rmdir
#define rmdir _rmdir
#endif
#ifndef access
#define access _access
#endif
#ifndef F_OK
#define F_OK 0
#endif
#ifndef R_OK
#define R_OK 4
#endif
#ifndef W_OK
#define W_OK 2
#endif
#ifndef X_OK
#define X_OK 0  /* Windows doesn't have execute permission like Unix */
#endif

#else
/* POSIX systems */
#include <unistd.h>
#endif

#endif /* COMPAT_UNISTD_H */
