#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>

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

// 分配对齐到内存页的缓冲区
char* align_alloc(size_t size) {
    // 获取系统内存页大小
    size_t page_size = io_blocksize();
    
    // 分配额外空间以确保能容纳对齐块和元数据
    size_t total_size = size + page_size + sizeof(void*);
    void* raw_mem = malloc(total_size);
    
    if (!raw_mem) {
        perror("malloc");
        return NULL;
    }
    
    // 计算对齐地址
    uintptr_t start_addr = (uintptr_t)raw_mem;
    uintptr_t aligned_addr = (start_addr + page_size - 1) & ~(page_size - 1);
    
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
    // 参数验证
    if (argc != 2) {
        dprintf(STDERR_FILENO, "Usage: %s <filename>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // 确定缓冲区大小
    size_t buffer_size = io_blocksize();
    char *buffer = align_alloc(buffer_size);
    
    if (!buffer) {
        exit(EXIT_FAILURE);
    }

    // 打开文件
    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("open");
        align_free(buffer);
        exit(EXIT_FAILURE);
    }

    ssize_t bytes_read;
    
    // 带缓冲区的读写循环
    while ((bytes_read = read(fd, buffer, buffer_size)) ){
        if (bytes_read == -1) {
            // 处理读取错误
            if (errno == EINTR) continue;
            perror("read");
            align_free(buffer);
            close(fd);
            exit(EXIT_FAILURE);
        }

        char *write_ptr = buffer;
        ssize_t bytes_remaining = bytes_read;
        
        // 确保完整写入所有读取的数据
        while (bytes_remaining > 0) {
            ssize_t bytes_written = write(STDOUT_FILENO, write_ptr, bytes_remaining);
            
            if (bytes_written == -1) {
                if (errno == EINTR) continue;
                perror("write");
                align_free(buffer);
                close(fd);
                exit(EXIT_FAILURE);
            }
            
            bytes_remaining -= bytes_written;
            write_ptr += bytes_written;
        }
    }

    // 清理资源
    align_free(buffer);
    
    if (close(fd) == -1) {
        perror("close");
        exit(EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
}