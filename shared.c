#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include "shared.h"

ssize_t
write2(int fd, void *buf, size_t count)
{
    size_t nleft, nwrite;
    ssize_t n;
    nleft = count;
    nwrite = 0;
    do {
        n = write(fd, (char *)buf + nwrite, nleft);
        if (n == 0)
            return (nwrite);
        else if (n == -1)
            return (-1);
        nwrite += n;
        nleft -= n;
    } while (nleft);
    return (nwrite);
}

int
read2(int fd, void *data, size_t len)
{
    int n, left, done;

    left = len;
    done = 0;

    while (left > 0) {
        n = read(fd, (unsigned char *)data + done, left);
        if (n == -1) {
            if (errno == EINTR)
                continue;
            return (-1);
        }
        done += n;
        left -= n;
    }

    return (0);
}

void *
loadfile(const char *fn, size_t * len)
{
    int fd, e;
    struct stat st;
    void *data;

    fd = open(fn, O_RDONLY);
    if (fd == -1)
        return (NULL);

    if (fstat(fd, &st) == -1) {
        e = errno;
        close(fd);
        errno = e;
        return (NULL);
    }

    *len = st.st_size;

    data = malloc(*len);
    if (data == NULL) {
        e = errno;
        close(fd);
        errno = e;
        return (NULL);
    }

    e = read2(fd, data, *len);
    if (e == -1) {
        e = errno;
        close(fd);
        free(data);
        errno = e;
        return (NULL);
    }

    close(fd);

    return (data);
}

int
savefile(const char *fn, void *data, size_t len)
{
    int fd, e;
    fd = open(fn, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1)
        return -1;
    e = write2(fd, data, len);
    if (e == -1) {
        e = errno;
        close(fd);
        errno = e;
        return -1;
    }
    close(fd);
    return 0;
}
