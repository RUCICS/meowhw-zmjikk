#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

size_t io_blocksize(void) {
    long page_size = sysconf(_SC_PAGESIZE);
    if (page_size == -1) {
        perror("sysconf(_SC_PAGESIZE)");
        return 4096;
    }
    return (size_t)page_size;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        const char *err_msg = "用法: mycat2 <文件>\n";
        write(STDERR_FILENO, err_msg, strlen(err_msg));
        exit(EXIT_FAILURE);
    }

    size_t buf_size = io_blocksize();
    char *buf = malloc(buf_size);
    if (buf == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("open");
        free(buf);
        exit(EXIT_FAILURE);
    }

    ssize_t nread;
    while ((nread = read(fd, buf, buf_size)) ){
        if (nread == -1) {
            if (errno == EINTR) continue;
            perror("read");
            close(fd);
            free(buf);
            exit(EXIT_FAILURE);
        }

        char *ptr = buf;
        ssize_t nwritten;
        while (nread > 0) {
            nwritten = write(STDOUT_FILENO, ptr, nread);
            if (nwritten == -1) {
                if (errno == EINTR) continue;
                perror("write");
                close(fd);
                free(buf);
                exit(EXIT_FAILURE);
            }
            nread -= nwritten;
            ptr += nwritten;
        }
    }

    if (close(fd) == -1) {
        perror("close");
        free(buf);
        exit(EXIT_FAILURE);
    }
    free(buf);

    return EXIT_SUCCESS;
}