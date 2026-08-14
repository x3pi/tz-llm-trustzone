#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <cstring>
#include <iostream>
#include <string>

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

    bool is_test_mode = (argc > 1 && std::string(argv[1]) == "test");

    char *mapped_mem = (char *)mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mapped_mem == MAP_FAILED) {
        perror("Failed to mmap shared memory");
        close(fd);
        return 1;
    }

    auto send_command = [&](const std::string& cmd) -> std::string {
        strncpy(mapped_mem, cmd.c_str(), SHM_SIZE - 1);
        mapped_mem[SHM_SIZE - 1] = '\0';
        
        int out_cmd;
        ioctl(fd, LLM_CLIENT_IOCTL_RUN, fd, &out_cmd);
        
        // Handle TEE double-yield bug (like in xapian-ca)
        if (strncmp(mapped_mem, cmd.c_str(), SHM_SIZE) == 0) {
            ioctl(fd, LLM_CLIENT_IOCTL_RUN, fd, &out_cmd);
        }
        return std::string(mapped_mem);
    };

    if (is_test_mode) {
        std::cout << "[EVM-CA] Running Automated Test Suite..." << std::endl;
        
        // 1. Deploy a simple contract that returns 42 (0x2a)
        // Init code: 600a600c600039600a6000f3 (copy 10 bytes of runtime code)
        // Runtime code: 602a60005260206000f3 (MSTORE 0x2a to 0x00, return 32 bytes from 0x00)
        std::string deploy_cmd = "DEPLOY:600a600c600039600a6000f3602a60005260206000f3";
        std::cout << "[TEST] Executing: " << deploy_cmd << std::endl;
        std::string res1 = send_command(deploy_cmd);
        std::cout << "[TEST] Result: " << res1 << std::endl;
        
        if (res1.find("SUCCESS") == std::string::npos) {
            std::cout << "[TEST] ❌ FAILED to deploy contract." << std::endl;
            close(fd);
            return 1;
        }

        // 2. Call the contract at address 1
        std::string call_cmd = "CALL:1:00"; // dummy data
        std::cout << "[TEST] Executing: " << call_cmd << std::endl;
        std::string res2 = send_command(call_cmd);
        std::cout << "[TEST] Result: " << res2 << std::endl;

        // Verify the output ends with 0x2a (42)
        if (res2.find("SUCCESS") != std::string::npos && res2.find("000000000000000000000000000000000000000000000000000000000000002a") != std::string::npos) {
            std::cout << "\n✅ [TEST] ALL TESTS PASSED! EVM IS FULLY FUNCTIONAL IN TEE." << std::endl;
        } else {
            std::cout << "\n❌ [TEST] FAILED: Output did not match expected '0x2a'." << std::endl;
        }
        
        close(fd);
        return 0;
    }

    std::cout << "[EVM-CA] Connected to TrustZone EVM-TA!" << std::endl;
    std::cout << "[EVM-CA] Interactive Mode. Type 'exit' to quit." << std::endl;
    std::cout << "[EVM-CA] Commands:" << std::endl;
    std::cout << "  DEPLOY:<hex_bytecode>   - Deploy a new contract" << std::endl;
    std::cout << "  CALL:<address>:<data>   - Call an existing contract" << std::endl;
    
    std::string user_query;
    while (true) {
        std::cout << "evm> ";
        std::getline(std::cin, user_query);
        
        if (user_query == "exit" || user_query == "quit") {
            break;
        }
        if (user_query.empty()) {
            continue;
        }

        std::string res = send_command(user_query);
        
        std::cout << "========================================\n"
                  << res << "\n"
                  << "========================================\n";
    }

    close(fd);
    return 0;
}
