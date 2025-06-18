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
    return (x & (x - 1)) == 0;
}

// 获取合理的I/O块大小
size_t io_blocksize(const char* filename) {
    // 获取内存页大小
    long page_size = sysconf(_SC_PAGESIZE);
    if (page_size == -1) {
        perror("sysconf(_SC_PAGESIZE)");
        page_size = 4096;
    }

    // 获取文件系统块大小
    struct stat st;
    if (stat(filename, &st) == -1) {
        perror("stat");
        return page_size;
    }

    size_t blksize = st.st_blksize;
    
    // 验证块大小的合理性
    if (blksize == 0 || !is_power_of_two(blksize)) {
        fprintf(stderr, "Warning: Invalid block size %zu, using page size %ld\n",
                blksize, page_size);
        return page_size;
    }

    // 取两者的最小公倍数
    size_t lcm = (page_size > blksize) ? page_size : blksize;
    while (lcm % page_size != 0 || lcm % blksize != 0) {
        lcm += (page_size > blksize) ? page_size : blksize;
    }

    // 限制最大缓冲区大小（4MB）
    const size_t max_buffer = 4 * 1024 * 1024;
    return lcm > max_buffer ? max_buffer : lcm;
}

// 对齐内存分配和释放函数保持不变（同mycat3）

int main(int argc, char *argv[]) {
    if (argc != 2) {
        dprintf(STDERR_FILENO, "Usage: %s <filename>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // 获取动态块大小
    size_t buffer_size = io_blocksize(argv[1]);
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

    // 读写循环保持不变（同mycat3）

    align_free(buffer);
    if (close(fd) == -1) {
        perror("close");
        exit(EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
}