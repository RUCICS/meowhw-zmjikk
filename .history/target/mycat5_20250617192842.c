#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/statvfs.h>
#include <stdint.h>

static size_t gcd(size_t a, size_t b) {
    while (b != 0) {
        size_t temp = b;
        b = a % b;
        a = temp;
    }
    return a;
}

static size_t lcm(size_t a, size_t b) {
    if (a == 0 || b == 0) return 0;
    return (a / gcd(a, b)) * b;
}

size_t io_blocksize(int fd) {
    long page_size = sysconf(_SC_PAGESIZE);
    if (page_size == -1) {
        perror("sysconf(_SC_PAGESIZE)");
        page_size = 4096;
    }

    struct statvfs vfs;
    size_t fs_block_size = page_size;
    
    if (fstatvfs(fd, &vfs) == 0) {
        fs_block_size = vfs.f_bsize;
        if ((fs_block_size & (fs_block_size - 1)) != 0) {
            if (fs_block_size < 512) fs_block_size = 512;
            else if (fs_block_size > 64 * 1024) fs_block_size = 64 * 1024;
            size_t power = 1;
            while (power < fs_block_size) power <<= 1;
            fs_block_size = power >> 1;
        }
    } else {
        perror("fstatvfs");
    }

    size_t block_size = lcm((size_t)page_size, fs_block_size);
    const size_t optimal_multiplier = 32;
    size_t optimal_size = block_size * optimal_multiplier;

    const size_t min_buffer = 4 * 1024;    // 4KB
    const size_t max_buffer = 4 * 1024 * 1024; // 4MB
    
    if (optimal_size < min_buffer) {
        optimal_size = min_buffer;
    } else if (optimal_size > max_buffer) {
        optimal_size = max_buffer;
    }

    if (optimal_size % page_size != 0) {
        optimal_size = ((optimal_size / page_size) + 1) * page_size;
    }
    
    return optimal_size;
}

char* align_alloc(size_t size) {
    long page_size = sysconf(_SC_PAGESIZE);
    if (page_size == -1) page_size = 4096;

    void *original = malloc(size + page_size - 1 + sizeof(void*));
    if (original == NULL) {
        perror("malloc");
        return NULL;
    }

    uintptr_t addr = (uintptr_t)original + sizeof(void*);
    uintptr_t offset = page_size - (addr % page_size);
    char *aligned = (char*)addr + offset;
    *((void**)(aligned - sizeof(void*))) = original;
    
    return aligned;
}

// 释放对齐分配的内存
void align_free(void* ptr) {
    if (ptr == NULL) return;
    void *original = *((void**)((char*)ptr - sizeof(void*)));
    free(original);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        const char *err_msg = "用法: mycat5 <文件>\n";
        write(STDERR_FILENO, err_msg, strlen(err_msg));
        exit(EXIT_FAILURE);
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    size_t buf_size = io_blocksize(fd);
    printf("使用缓冲区大小: %zu 字节 (%.2f KB)\n", 
           buf_size, (double)buf_size / 1024);

    char *buf = align_alloc(buf_size);
    if (buf == NULL) {
        fprintf(stderr, "无法分配对齐缓冲区\n");
        close(fd);
        exit(EXIT_FAILURE);
    }

    ssize_t nread;
    while ((nread = read(fd, buf, buf_size))) {
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

    if (close(fd) == -1) {
        perror("close");
        align_free(buf);
        exit(EXIT_FAILURE);
    }
    align_free(buf);

    return EXIT_SUCCESS;
}