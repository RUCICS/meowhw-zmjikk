#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/mman.h>

// 获取系统内存页大小
size_t io_blocksize(void) {
    long page_size = sysconf(_SC_PAGESIZE);
    if (page_size == -1) {
        perror("sysconf(_SC_PAGESIZE)");
        return 4096; // 默认页大小
    }
    return (size_t)page_size;
}

// 分配对齐到内存页的缓冲区
char* align_alloc(size_t size) {
    size_t page_size = io_blocksize();
    
    // 分配额外空间以确保有足够的对齐区域
    void *mem = malloc(size + page_size - 1);
    if (mem == NULL) {
        perror("malloc");
        return NULL;
    }
    
    // 计算对齐地址
    uintptr_t addr = (uintptr_t)mem;
    uintptr_t offset = page_size - (addr % page_size);
    char *aligned = (char *)mem + offset;
    
    // 存储原始指针以便释放
    *((void **)aligned - 1) = mem;
    
    return aligned;
}

// 释放对齐分配的内存
void align_free(void* ptr) {
    if (ptr == NULL) return;
    
    // 从对齐指针前的位置获取原始指针
    void *original = *((void **)ptr - 1);
    free(original);
}

int main(int argc, char *argv[]) {
    // 检查参数数量
    if (argc != 2) {
        const char *err_msg = "用法: mycat3 <文件>\n";
        write(STDERR_FILENO, err_msg, strlen(err_msg));
        exit(EXIT_FAILURE);
    }

    // 获取内存页大小
    size_t page_size = io_blocksize();
    printf("使用页大小: %zu 字节\n", page_size);
    
    // 分配对齐的缓冲区
    char *buf = align_alloc(page_size);
    if (buf == NULL) {
        fprintf(stderr, "无法分配对齐缓冲区\n");
        exit(EXIT_FAILURE);
    }

    // 打开文件
    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("open");
        align_free(buf);
        exit(EXIT_FAILURE);
    }

    // 带缓冲区的读写
    ssize_t nread;
    while ((nread = read(fd, buf, page_size)) {
        if (nread == -1) {
            if (errno == EINTR) continue; // 被信号中断，重试
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
                if (errno == EINTR) continue; // 被信号中断，重试
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