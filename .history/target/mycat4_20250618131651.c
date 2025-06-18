#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <stdint.h>

// 判断是否为2的幂
static int is_power_of_two(size_t n) {
    return (n != 0) && ((n & (n - 1))) == 0;
}

// 获取系统内存页大小
size_t get_page_size(void) {
    static long page_size = 0;
    if (page_size == 0) {
        page_size = sysconf(_SC_PAGESIZE);
        if (page_size <= 0) {
            fprintf(stderr, "Warning: Failed to get page size (%s), using 4096\n", 
                    strerror(errno));
            page_size = 4096;
        }
    }
    return (size_t)page_size;
}

// 获取最佳IO块大小（考虑内存页和文件系统块大小）
size_t io_blocksize(int fd) {
    struct stat st;
    
    // 获取文件系统块大小
    if (fstat(fd, &st)) {
        perror("fstat");
        return get_page_size(); // 回退到页大小
    }
    
    size_t fs_block_size = st.st_blksize;
    size_t page_size = get_page_size();
    
    // 验证文件系统块大小的有效性
    if (fs_block_size == 0 || !is_power_of_two(fs_block_size)) {
        fprintf(stderr, "Warning: Invalid filesystem block size (%zu), using page size\n", 
                fs_block_size);
        return page_size;
    }
    
    // 计算合理的缓冲区大小（文件系统块大小的倍数）
    size_t buf_size = fs_block_size;
    
    // 确保至少是内存页大小
    if (buf_size < page_size) {
        buf_size = page_size;
        // 调整为文件系统块大小的倍数
        if (page_size % fs_block_size != 0) {
            buf_size = ((page_size / fs_block_size) + 1) * fs_block_size;
        }
    }
    
    // 设置上限（1MB）
    const size_t max_buffer = 1 << 20; // 1MB
    if (buf_size > max_buffer) {
        buf_size = max_buffer;
        // 调整为文件系统块大小的倍数
        buf_size = (buf_size / fs_block_size) * fs_block_size;
    }
    
    return buf_size;
}

// 分配页面对齐的内存
void* align_alloc(size_t size) {
    size_t page_size = get_page_size();
    
    // 分配额外空间以确保可以对齐
    void* raw_ptr = malloc(size + page_size - 1 + sizeof(void*));
    if (!raw_ptr) {
        return NULL;
    }
    
    // 计算对齐的起始地址
    uintptr_t raw_addr = (uintptr_t)raw_ptr;
    uintptr_t aligned_addr = (raw_addr + sizeof(void*) + page_size - 1) & ~(page_size - 1);
    
    // 存储原始指针用于释放
    void** storage = (void**)(aligned_addr - sizeof(void*));
    *storage = raw_ptr;
    
    return (void*)aligned_addr;
}

// 释放页面对齐的内存
void align_free(void* ptr) {
    if (!ptr) return;
    
    // 获取存储的原始指针
    void** storage = (void**)((uintptr_t)ptr - sizeof(void*));
    void* raw_ptr = *storage;
    
    free(raw_ptr);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // 打开文件
    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    // 获取最佳IO块大小（基于文件系统）
    size_t buf_size = io_blocksize(fd);
    
    // 分配页面对齐的缓冲区
    char *buffer = align_alloc(buf_size);
    if (!buffer) {
        perror("align_alloc");
        close(fd);
        exit(EXIT_FAILURE);
    }

    ssize_t bytes_read;
    while ((bytes_read = read(fd, buffer, buf_size))){
        if (bytes_read == -1) {
            perror("read");
            close(fd);
            align_free(buffer);
            exit(EXIT_FAILURE);
        }
        
        if (bytes_read == 0) break; // EOF
        
        ssize_t bytes_written = 0;
        while (bytes_written < bytes_read) {
            ssize_t n = write(STDOUT_FILENO, 
                             buffer + bytes_written, 
                             bytes_read - bytes_written);
            if (n == -1) {
                perror("write");
                close(fd);
                align_free(buffer);
                exit(EXIT_FAILURE);
            }
            bytes_written += n;
        }
    }

    if (close(fd)) {
        perror("close");
        align_free(buffer);
        exit(EXIT_FAILURE);
    }
    
    align_free(buffer);
    return EXIT_SUCCESS;
}