#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <errno.h>

// 获取内存页大小
size_t get_page_size() {
    long page_size = sysconf(_SC_PAGESIZE);
    if (page_size < 0) {
        perror("sysconf");
        exit(1);
    }
    return (size_t)page_size;
}

// 获取文件系统块大小
size_t get_fs_block_size(const char *filename) {
    struct statvfs st;
    if (statvfs(filename, &st) < 0) {
        perror("statvfs");
        exit(1);
    }
    size_t fs_block_size = st.f_bsize;
    if (fs_block_size & (fs_block_size - 1)) {
        size_t adjusted = 1;
        while (adjusted < fs_block_size) {
            adjusted <<= 1;
        }
        fs_block_size = adjusted;
    }
    return fs_block_size;
}

// 设置缓冲区大小
size_t io_blocksize(const char *filename) {
    size_t page_size = get_page_size();
    size_t fs_block_size = get_fs_block_size(filename);
    size_t base_size = (page_size > fs_block_size) ? page_size : fs_block_size;
    return base_size * 64; // 使用实验得出的最优倍数
}

// 对齐内存分配
char* align_alloc(size_t size) {
    void *ptr;
    size_t page_size = get_page_size();
    int ret = posix_memalign(&ptr, page_size, size);
    if (ret != 0) {
        errno = ret;
        perror("posix_memalign");
        exit(1);
    }
    return (char*)ptr;
}

// 释放对齐内存
void align_free(void *ptr) {
    if (ptr) {
        free(ptr);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        exit(1);
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("open");
        exit(1);
    }

    size_t buf_size = io_blocksize(argv[1]);
    char *buf = align_alloc(buf_size);
    if (!buf) {
        perror("align_alloc");
        exit(1);
    }

    ssize_t n;
    while ((n = read(fd, buf, buf_size)) > 0) {
        if (write(STDOUT_FILENO, buf, n) != n) {
            perror("write");
            exit(1);
        }
    }

    if (n < 0) {
        perror("read");
        exit(1);
    }

    align_free(buf);
    if (close(fd) < 0) {
        perror("close");
        exit(1);
    }

    return 0;
}