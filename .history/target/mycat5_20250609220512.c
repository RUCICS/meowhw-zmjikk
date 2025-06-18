#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>

// 实验确定的最佳倍数
#define OPTIMAL_MULTIPLIER 16

static int is_power_of_two(size_t x) {
    return (x != 0) && ((x & (x - 1)) == 0);
}

static size_t get_page_size() {
    static size_t page_size = 0;
    if (page_size == 0) {
        long result = sysconf(_SC_PAGESIZE);
        page_size = (result == -1) ? 4096 : (size_t)result;
    }
    return page_size;
}

size_t io_blocksize(const char* filename) {
    size_t base_size = get_page_size();
    
    // 获取文件系统块大小（简化版，假设大部分文件一致）
    struct stat st;
    if (stat(filename, &st) == 0 && st.st_blksize > 0 && 
        is_power_of_two(st.st_blksize)) {
        base_size = st.st_blksize;
    }

    // 应用实验得出的最优倍数
    size_t optimal_size = base_size * OPTIMAL_MULTIPLIER;
    
    // 限制在256KB-1MB范围内
    const size_t min_buffer = 256 * 1024;
    const size_t max_buffer = 1024 * 1024;
    
    if (optimal_size < min_buffer) return min_buffer;
    if (optimal_size > max_buffer) return max_buffer;
    return optimal_size;
}

char* align_alloc(size_t size) {
    size_t page_size = get_page_size();
    size_t total_size = size + page_size + sizeof(void*);
    
    void* raw_mem = malloc(total_size);
    if (!raw_mem) {
        perror("malloc");
        return NULL;
    }
    
    uintptr_t start_addr = (uintptr_t)raw_mem;
    uintptr_t aligned_addr = (start_addr + page_size + sizeof(void*) - 1) & ~(page_size - 1);
    
    void** meta_ptr = (void**)(aligned_addr - sizeof(void*));
    *meta_ptr = raw_mem;
    
    return (char*)aligned_addr;
}

void align_free(void* ptr) {
    if (!ptr) return;
    
    void** meta_ptr = (void**)((uintptr_t)ptr - sizeof(void*));
    void* raw_mem = *meta_ptr;
    
    free(raw_mem);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        dprintf(STDERR_FILENO, "Usage: %s <filename>\n", argv[0]);
        return EXIT_FAILURE;
    }

    size_t buffer_size = io_blocksize(argv[1]);
    printf("Using buffer size: %.2f KB\n", buffer_size / 1024.0); // Debug info
    
    char *buffer = align_alloc(buffer_size);
    if (!buffer) {
        return EXIT_FAILURE;
    }

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