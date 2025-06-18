#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

// 获取系统内存页大小
size_t io_blocksize(void) {
    long page_size = sysconf(_SC_PAGESIZE);
    
    if (page_size == -1) {
        perror("sysconf(_SC_PAGESIZE)");
        fprintf(stderr, "Warning: Using default block size 4096\n");
        return 4096;
    }
    
    return (size_t)page_size;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        dprintf(STDERR_FILENO, "Usage: %s <filename>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    size_t buffer_size = io_blocksize();
    char *buffer = malloc(buffer_size);
    
    if (!buffer) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("open");
        free(buffer);
        exit(EXIT_FAILURE);
    }

    ssize_t bytes_read;
    while ((bytes_read = read(fd, buffer, buffer_size))) {
        if (bytes_read == -1) {
            // 处理读取错误
            if (errno == EINTR) continue;
            perror("read");
            free(buffer);
            close(fd);
            exit(EXIT_FAILURE);
        }

        char *write_ptr = buffer;
        ssize_t bytes_remaining = bytes_read;

        while (bytes_remaining > 0) {
            ssize_t bytes_written = write(STDOUT_FILENO, write_ptr, bytes_remaining);
            
            if (bytes_written == -1) {
                if (errno == EINTR) continue;
                perror("write");
                free(buffer);
                close(fd);
                exit(EXIT_FAILURE);
            }
            
            bytes_remaining -= bytes_written;
            write_ptr += bytes_written;
        }
    }

    free(buffer);
    
    if (close(fd) == -1) {
        perror("close");
        exit(EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
}