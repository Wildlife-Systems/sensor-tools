/*
 * Cross-platform stat compatibility header
 */
#ifndef COMPAT_STAT_H
#define COMPAT_STAT_H

#include <sys/stat.h>

#if defined(_WIN32) || defined(_WIN64)
/* Windows uses _stat instead of stat */
#ifndef S_ISDIR
#define S_ISDIR(mode) (((mode) & _S_IFMT) == _S_IFDIR)
#endif
#ifndef S_ISREG
#define S_ISREG(mode) (((mode) & _S_IFMT) == _S_IFREG)
#endif
#endif

#endif /* COMPAT_STAT_H */
