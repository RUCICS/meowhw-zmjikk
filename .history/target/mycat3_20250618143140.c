#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

size_t get_page_size() {
    long page_size = sysconf(_SC_PAGESIZE);
    if (page_size < 0) {
        perror("sysconf");
        exit(1);
    }
    return (size_t)page_size;
}

char* align_alloc(size_t size) {
    void *ptr;
    size_t page_size = get_page_size();
    int ret = posix_memalign(&ptr, page_size, size);
    if (ret != 0) {
        errno = ret;
        perror("posix_memalign");
        exit(1);
    }
    return (char*)ptr;
}

void align_free(void *ptr) {
    if (ptr) {
        free(ptr);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        exit(1);
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("open");
        exit(1);
    }

    size_t buf_size = get_page_size();
    char *buf = align_alloc(buf_size);
    if (!buf) {
        perror("align_alloc");
        exit(1);
    }

    ssize_t n;
    while ((n = read(fd, buf, buf_size)) > 0) {
        if (write(STDOUT_FILENO, buf, n) != n) {
            perror("write");
            exit(1);
        }
    }

    if (n < 0) {
        perror("read");
        exit(1);
    }

    align_free(buf);
    if (close(fd) < 0) {
        perror("close");
        exit(1);
    }

    return 0;
}