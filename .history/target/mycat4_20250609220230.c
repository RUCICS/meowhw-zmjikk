#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <math.h>

// 判断是否为2的整数次幂
static int is_power_of_two(size_t x) {
    return (x != 0) && ((x & (x - 1)) == 0);
}

// 获取系统内存页大小
static size_t get_page_size() {
    static size_t page_size = 0;
    if (page_size == 0) {
        long result = sysconf(_SC_PAGESIZE);
        page_size = (result == -1) ? 4096 : (size_t)result;
    }
    return page_size;
}

// 获取合理的I/O块大小
size_t io_blocksize(const char* filename) {
    size_t page_size = get_page_size();
    
    // 获取文件系统块大小
    struct stat st;
    if (stat(filename, &st) == -1) {
        perror("stat");
        fprintf(stderr, "Warning: Using page size %zu as block size\n", page_size);
        return page_size;
    }

    size_t blksize = st.st_blksize;
    
    // 验证块大小的合理性
    if (blksize == 0 || !is_power_of_two(blksize)) {
        fprintf(stderr, "Warning: Invalid block size %zu, using page size %zu\n",
                blksize, page_size);
        return page_size;
    }

    // 计算最小公倍数
    size_t larger = (page_size > blksize) ? page_size : blksize;
    size_t smaller = (page_size < blksize) ? page_size : blksize;
    size_t lcm = larger;
    while (lcm % smaller != 0) {
        lcm += larger;
    }

    // 限制最大缓冲区大小（4MB）
    const size_t max_buffer = 4 * 1024 * 1024;
    return (lcm > max_buffer) ? max_buffer : lcm;
}

// 分配对齐到内存页的缓冲区
char* align_alloc(size_t size) {
    size_t page_size = get_page_size();
    size_t total_size = size + page_size + sizeof(void*);
    
    void* raw_mem = malloc(total_size);
    if (!raw_mem) {
        perror("malloc");
        return NULL;
    }
    
    // 计算对齐地址
    uintptr_t start_addr = (uintptr_t)raw_mem;
    uintptr_t aligned_addr = (start_addr + page_size + sizeof(void*) - 1) & ~(page_size - 1);
    
    // 在对齐地址前存储原始指针
    void** meta_ptr = (void**)(aligned_addr - sizeof(void*));
    *meta_ptr = raw_mem;
    
    return (char*)aligned_addr;
}

// 释放对齐分配的内存
void align_free(void* ptr) {
    if (!ptr) return;
    
    // 从对齐指针前获取原始指针
    void** meta_ptr = (void**)((uintptr_t)ptr - sizeof(void*));
    void* raw_mem = *meta_ptr;
    
    free(raw_mem);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        dprintf(STDERR_FILENO, "Usage: %s <filename>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // 获取动态块大小
    size_t buffer_size = io_blocksize(argv[1]);
    char *buffer = align_alloc(buffer_size);
    if (!buffer) {
        return EXIT_FAILURE;
    }

    // 打开文件
    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("open");
        align_free(buffer);
        return EXIT_FAILURE;
    }

    ssize_t bytes_read;
    while ((bytes_read = read(fd, buffer, buffer_size)) > 0) {
        char *write_ptr = buffer;
        ssize_t bytes_remaining = bytes_read;
        
        while (bytes_remaining > 0) {
            ssize_t bytes_written = write(STDOUT_FILENO, write_ptr, bytes_remaining);
            if (bytes_written == -1) {
                if (errno == EINTR) continue;
                perror("write");
                align_free(buffer);
                close(fd);
                return EXIT_FAILURE;
            }
            bytes_remaining -= bytes_written;
            write_ptr += bytes_written;
        }
    }

    if (bytes_read == -1) {
        perror("read");
        align_free(buffer);
        close(fd);
        return EXIT_FAILURE;
    }

    align_free(buffer);
    if (close(fd) == -1) {
        perror("close");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}