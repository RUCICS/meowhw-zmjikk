#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/mman.h>
#include <stdint.h>

size_t io_blocksize(void) {
    long page_size = sysconf(_SC_PAGESIZE);
    if (page_size == -1) {
        perror("sysconf(_SC_PAGESIZE)");
        return 4096;
    }
    return (size_t)page_size;
}

char* align_alloc(size_t size) {
    size_t page_size = io_blocksize();
    void *mem = malloc(size + page_size - 1);
    if (mem == NULL) {
        perror("malloc");
        return NULL;
    }
    uintptr_t addr = (uintptr_t)mem;
    uintptr_t offset = page_size - (addr % page_size);
    char *aligned = (char *)mem + offset;
    *((void **)aligned - 1) = mem;
    
    return aligned;
}

void align_free(void* ptr) {
    if (ptr == NULL) return;
    void *original = *((void **)ptr - 1);
    free(original);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        const char *err_msg = "用法: mycat3 <文件>\n";
        write(STDERR_FILENO, err_msg, strlen(err_msg));
        exit(EXIT_FAILURE);
    }

    size_t page_size = io_blocksize();
    printf("使用页大小: %zu 字节\n", page_size);

    char *buf = align_alloc(page_size);
    if (buf == NULL) {
        fprintf(stderr, "无法分配对齐缓冲区\n");
        exit(EXIT_FAILURE);
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("open");
        align_free(buf);
        exit(EXIT_FAILURE);
    }

    ssize_t nread;
    while ((nread = read(fd, buf, page_size))) {
        if (nread == -1) {
            if (errno == EINTR) continue;
            perror("read");
            close(fd);
            align_free(buf);
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
                align_free(buf);
                exit(EXIT_FAILURE);
            }
            nread -= nwritten;
            ptr += nwritten;
        }
    }

    // 清理资源
    if (close(fd) == -1) {
        perror("close");
        align_free(buf);
        exit(EXIT_FAILURE);
    }
    align_free(buf);

    return EXIT_SUCCESS;
}