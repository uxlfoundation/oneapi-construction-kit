// ----- utimensat.h -----
#include <sys/stat.h>
#ifdef __cplusplus
extern "C" {
#endif
int utimensat(int dirfd, const char *pathname,
        const struct timespec times[2], int flags);
int futimens(int fd, const struct timespec times[2]);
#ifdef __cplusplus
}
#endif
