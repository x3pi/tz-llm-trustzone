#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <cstring>
#include <iostream>

#define DEVICE_NAME "/dev/tc_ns_client"
#define TC_NS_CLIENT_IOC_MAGIC  't'
#define LLM_CLIENT_IOCTL_RUN \
	_IOWR(TC_NS_CLIENT_IOC_MAGIC, 24, int)

#define SHM_SIZE (1 * 1024 * 1024)

int main(int argc, char *argv[]) {
    int fd = open(DEVICE_NAME, O_RDWR);
    if (fd < 0) {
        perror("Failed to open /dev/tc_ns_client");
        return 1;
    }

    char *mapped_mem = (char *)mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mapped_mem == MAP_FAILED) {
        perror("Failed to mmap shared memory");
        close(fd);
        return 1;
    }

    // Interactive shell loop
    std::string user_query;
    if (argc > 1) {
        // Run once mode (for the first query)
        user_query = argv[1];
        std::cout << "[Xapian-CA] Sending query to TEE: '" << user_query << "'" << std::endl;
        strncpy(mapped_mem, user_query.c_str(), SHM_SIZE - 1);
        
        int out_cmd;
        ioctl(fd, LLM_CLIENT_IOCTL_RUN, fd, &out_cmd);
        
        std::cout << "[Xapian-CA] SMC Returned! Result from TEE:\n"
                  << "========================================\n"
                  << mapped_mem << "\n"
                  << "========================================\n";
    } else {
        // Interactive mode
        std::cout << "[Xapian-CA] Entering Interactive Mode. Type 'exit' to quit." << std::endl;
        while (true) {
            std::cout << "xapian> ";
            std::getline(std::cin, user_query);
            
            if (user_query == "exit" || user_query == "quit") {
                break;
            }
            if (user_query.empty()) {
                continue;
            }

            strncpy(mapped_mem, user_query.c_str(), SHM_SIZE - 1);
            
            int out_cmd;
            ioctl(fd, LLM_CLIENT_IOCTL_RUN, fd, &out_cmd);
            
            // Xử lý lỗi lệch nhịp của TEE (TA gọi 2 lệnh Yield)
            // Nếu TEE bỏ qua vòng lặp, kết quả trả về sẽ y hệt chuỗi truy vấn đầu vào.
            // Lúc này ta gọi ioctl thêm 1 lần nữa để ép TEE xử lý!
            if (strncmp(mapped_mem, user_query.c_str(), SHM_SIZE) == 0) {
                ioctl(fd, LLM_CLIENT_IOCTL_RUN, fd, &out_cmd);
            }
            
            std::cout << "========================================\n"
                      << mapped_mem << "\n"
                      << "========================================\n";
        }
    }

    close(fd);
    return 0;
}
