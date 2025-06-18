#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/statvfs.h>
#include <sys/mman.h>
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
    if (fstatvfs(fd, &vfs) == -1) {
        perror("fstatvfs");
        return (size_t)page_size;
    }
    
    size_t fs_block_size = vfs.f_bsize;
    
    // 处理虚假块大小（非2的幂）
    if ((fs_block_size & (fs_block_size - 1)) != 0) {
        if (fs_block_size < 512) fs_block_size = 512;
        else if (fs_block_size > 64 * 1024) fs_block_size = 64 * 1024;
        size_t power = 1;
        while (power < fs_block_size) power <<= 1;
        fs_block_size = power >> 1;
    }

    size_t block_size = lcm((size_t)page_size, fs_block_size);

    const size_t max_buffer = 1024 * 1024;
    if (block_size == 0 || block_size > max_buffer) {
        block_size = max_buffer;
    }

    if (block_size % page_size != 0) {
        block_size = ((block_size / page_size) + 1) * page_size;
    }
    
    return block_size;
}

// 分配对齐到内存页的缓冲区
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
        const char *err_msg = "用法: mycat4 <文件>\n";
        write(STDERR_FILENO, err_msg, strlen(err_msg));
        exit(EXIT_FAILURE);
    }

    // 打开文件
    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    size_t buf_size = io_blocksize(fd);
    printf("使用缓冲区大小: %zu 字节\n", buf_size);
    char *buf = align_alloc(buf_size);
    if (buf == NULL) {
        fprintf(stderr, "无法分配对齐缓冲区\n");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // 带缓冲区的读写
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