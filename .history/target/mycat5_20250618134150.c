#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <errno.h>

size_t get_page_size() {
    long page_size = sysconf(_SC_PAGESIZE);
    if (page_size < 0) {
        perror("sysconf");
        exit(1);
    }
    return (size_t)page_size;
}

size_t get_fs_block_size(const char *filename) {
    struct statvfs st;
    if (statvfs(filename, &st) < 0) {
        perror("statvfs");
        exit(1);
    }
    return st.f_bsize;
}

size_t io_blocksize(const char *filename) {
    size_t page_size = get_page_size();
    size_t fs_block_size = get_fs_block_size(filename);
    size_t base_size = (page_size > fs_block_size) ? page_size : fs_block_size;
    return base_size * 32; // 假设实验得出 32 为最优倍数
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

    size_t buf_size = io_blocksize(argv[1]);
    char *buf = malloc(buf_size);
    if (!buf) {
        perror("malloc");
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

    free(buf);
    if (close(fd) < 0) {
        perror("close");
        exit(1);
    }

    return 0;
}