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

    const char* query = "TrustZone";
    if (argc > 1) {
        query = argv[1];
    }

    // 1. Write query to shared memory
    std::cout << "[Xapian-CA] Sending query to TEE: '" << query << "'" << std::endl;
    strncpy(mapped_mem, query, SHM_SIZE - 1);

    // 2. Trigger SMC (wake up TEE)
    int out_cmd;
    std::cout << "[Xapian-CA] Triggering SMC..." << std::endl;
    unsigned long ret = ioctl(fd, LLM_CLIENT_IOCTL_RUN, fd, &out_cmd);
    
    if (ret != 0) {
        perror("[Xapian-CA] ioctl LLM_CLIENT_IOCTL_RUN failed");
    }

    // 3. Print Result from TEE
    std::cout << "[Xapian-CA] SMC Returned! Result from TEE:\n"
              << "========================================\n"
              << mapped_mem << "\n"
              << "========================================\n";

    close(fd);
    return 0;
}
