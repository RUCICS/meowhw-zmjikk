#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/statvfs.h>
#include <sys/mman.h>

// 计算最大公约数
static size_t gcd(size_t a, size_t b) {
    while (b != 0) {
        size_t temp = b;
        b = a % b;
        a = temp;
    }
    return a;
}

// 计算最小公倍数
static size_t lcm(size_t a, size_t b) {
    if (a == 0 || b == 0) return 0;
    return (a / gcd(a, b)) * b;
}

// 获取最优块大小
size_t io_blocksize(int fd) {
    // 获取内存页大小
    long page_size = sysconf(_SC_PAGESIZE);
    if (page_size == -1) {
        perror("sysconf(_SC_PAGESIZE)");
        page_size = 4096;
    }
    
    // 获取文件系统块大小
    struct statvfs vfs;
    if (fstatvfs(fd, &vfs) == -1) {
        perror("fstatvfs");
        // 回退到页大小
        return (size_t)page_size;
    }
    
    size_t fs_block_size = vfs.f_bsize;
    
    // 处理虚假块大小（非2的幂）
    if ((fs_block_size & (fs_block_size - 1)) != 0) {
        // 非2的幂，使用启发式方法
        if (fs_block_size < 512) fs_block_size = 512;
        else if (fs_block_size > 64 * 1024) fs_block_size = 64 * 1024;
        
        // 对齐到最近的2的幂
        size_t power = 1;
        while (power < fs_block_size) power <<= 1;
        fs_block_size = power >> 1; // 取小于等于原值的最大的2的幂
    }
    
    // 计算页大小和文件块大小的最小公倍数
    size_t block_size = lcm((size_t)page_size, fs_block_size);
    
    // 设置合理的上限（1MB）
    const size_t max_buffer = 1024 * 1024;
    if (block_size == 0 || block_size > max_buffer) {
        block_size = max_buffer;
    }
    
    // 确保是页大小的倍数（对齐要求）
    if (block_size % page_size != 0) {
        block_size = ((block_size / page_size) + 1) * page_size;
    }
    
    return block_size;
}

// 分配对齐到内存页的缓冲区
char* align_alloc(size_t size) {
    size_t page_size = sysconf(_SC_PAGESIZE);
    if (page_size == -1) page_size = 4096;
    
    // 分配额外空间用于存储原始指针和确保对齐
    void *original = malloc(size + page_size - 1 + sizeof(void*));
    if (original == NULL) {
        perror("malloc");
        return NULL;
    }
    
    // 计算对齐地址（跳过指针存储空间）
    uintptr_t addr = (uintptr_t)original + sizeof(void*);
    uintptr_t offset = page_size - (addr % page_size);
    char *aligned = (char*)addr + offset;
    
    // 在对齐指针前存储原始指针
    *((void**)(aligned - sizeof(void*))) = original;
    
    return aligned;
}

// 释放对齐分配的内存
void align_free(void* ptr) {
    if (ptr == NULL) return;
    
    // 从对齐指针前获取存储的原始指针
    void *original = *((void**)((char*)ptr - sizeof(void*)));
    free(original);
}

int main(int argc, char *argv[]) {
    // 检查参数数量
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

    // 获取最优块大小
    size_t buf_size = io_blocksize(fd);
    printf("使用缓冲区大小: %zu 字节\n", buf_size);
    
    // 分配对齐的缓冲区
    char *buf = align_alloc(buf_size);
    if (buf == NULL) {
        fprintf(stderr, "无法分配对齐缓冲区\n");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // 带缓冲区的读写
    ssize_t nread;
    while ((nread = read(fd, buf, buf_size)) {
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