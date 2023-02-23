// ----- utimensat.c -----
#include <sys/syscall.h>
#include "utimensat.h"
int utimensat(int dirfd, const char *pathname,
        const struct timespec times[2], int flags) {
    return syscall(__NR_utimensat, dirfd, pathname, times, flags);
}
int futimens(int fd, const struct timespec times[2]) {
    return utimensat(fd, NULL, times, 0);
}