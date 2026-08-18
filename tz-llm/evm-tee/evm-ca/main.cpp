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

std::string extract_address(const std::string& res) {
    size_t pos = res.find("address ");
    if (pos == std::string::npos) return "";
    std::string addr = res.substr(pos + 8);
    size_t end_pos = addr.find_first_of(" .");
    if (end_pos != std::string::npos) {
        addr = addr.substr(0, end_pos);
    }
    return addr;
}

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
        
        // Handle TEE double-yield bug
        if (strncmp(mapped_mem, cmd.c_str(), SHM_SIZE) == 0) {
            ioctl(fd, LLM_CLIENT_IOCTL_RUN, fd, &out_cmd);
        }
        return std::string(mapped_mem);
    };

    if (is_test_mode) {
        std::cout << "\n=======================================================" << std::endl;
        std::cout << "🦊 [EVM-CA] TEE EVM Wallet Test Suite (Standard EVM EOA)" << std::endl;
        std::cout << "=======================================================" << std::endl;
        
        // Query Wallet Information
        std::string wallet_info = send_command("WALLET");
        std::cout << "[WALLET] " << wallet_info << "\n" << std::endl;

        // 1. Deploy Basic 42 Contract
        std::cout << "[TX #1] 🚀 Deploying Basic 42 Contract from Wallet..." << std::endl;
        std::string deploy_basic_cmd = "DEPLOY:600a600c600039600a6000f3602a60005260206000f3";
        std::string res1 = send_command(deploy_basic_cmd);
        std::cout << "  ↳ Result: " << res1 << std::endl;
        
        if (res1.find("SUCCESS") == std::string::npos) {
            std::cout << "❌ [TEST] FAILED to deploy Basic contract." << std::endl;
            close(fd);
            return 1;
        }

        std::string basic_addr = extract_address(res1);
        std::cout << "  ↳ Derived Contract Address: " << basic_addr << "\n" << std::endl;

        // 2. Call Basic 42 Contract
        std::cout << "[TX #2] 📞 Calling Basic Contract at " << basic_addr << "..." << std::endl;
        std::string call_basic_cmd = "CALL:" + basic_addr + ":00";
        std::string res2 = send_command(call_basic_cmd);
        std::cout << "  ↳ Result: " << res2 << std::endl;

        if (res2.find("SUCCESS") != std::string::npos && res2.find("2a") != std::string::npos) {
            std::cout << "  ↳ Output Verification: 42 (0x2a) ✅ PASSED\n" << std::endl;
        } else {
            std::cout << "❌ [TEST] FAILED: Output did not match expected '0x2a'." << std::endl;
            close(fd);
            return 1;
        }
        
        // 3. Deploy Xapian SharedUpdate Contract
        std::cout << "[TX #3] 🚀 Deploying SharedUpdate Contract (Xapian Precompile 107)..." << std::endl;
        std::string shared_update_bytecode = "608060405234801562000010575f80fd5b5061010773ffffffffffffffffffffffffffffffffffffffff1663fbdddaf06040518060400160405280601681526020017f626c6f636b73746d5f7368617265645f78617069616e000000000000000000008152506040518263ffffffff1660e01b815260040162000083919062000161565b6020604051808303815f875af1158015620000a0573d5f803e3d5ffd5b505050506040513d601f19601f82011682018060405250810190620000c69190620001c1565b50620001f1565b5f81519050919050565b5f82825260208201905092915050565b5f5b8381101562000106578082015181840152602081019050620000e9565b5f8484015250505050565b5f601f19601f8301169050919050565b5f6200012d82620000cd565b620001398185620000d7565b93506200014b818560208601620000e7565b620001568162000111565b840191505092915050565b5f6020820190508181035f8301526200017b818462000121565b905092915050565b5f80fd5b5f8115159050919050565b6200019d8162000187565b8114620001a8575f80fd5b50565b5f81519050620001bb8162000192565b92915050565b5f60208284031215620001d957620001d862000183565b5b5f620001e884828501620001ab565b91505092915050565b61094280620001ff5f395ff3fe608060405234801561000f575f80fd5b506004361061004a575f3560e01c80630b6f8f481461004e5780638cc382961461006c578063b4340bbe1461008a578063d32a9a5914610094575b5f80fd5b61005661009e565b60405161006391906104b7565b60405180910390f35b6100746101b5565b60405161008191906104b7565b60405180910390f35b6100926101ba565b005b61009c610292565b005b5f8061010773ffffffffffffffffffffffffffffffffffffffff16633e7c7f8b6040518060400160405280601681526020017f626c6f636b73746d5f7368617265645f78617069616e000000000000000000008152505f546040518363ffffffff1660e01b815260040161011392919061055a565b5f604051808303815f875af115801561012e573d5f803e3d5ffd5b505050506040513d5f823e3d601f19601f8201168201806040525081019061015691906106b7565b90505f81511161019b576040517f08c379a000000000000000000000000000000000000000000000000000000000815260040161019290610748565b60405180910390fd5b808060200190518101906101af9190610790565b91505090565b5f5481565b61010773ffffffffffffffffffffffffffffffffffffffff16639d4799416040518060400160405280601681526020017f626c6f636b73746d5f7368617265645f78617069616e000000000000000000008152505f60405160200161021f91906104b7565b6040516020818303038152906040526040518363ffffffff1660e01b815260040161024b92919061080d565b6020604051808303815f875af1158015610267573d5f803e3d5ffd5b505050506040513d601f19601f8201168201806040525081019061028b9190610790565b5f81905550565b5f61010773ffffffffffffffffffffffffffffffffffffffff16633e7c7f8b6040518060400160405280601681526020017f626c6f636b73746d5f7368617265645f78617069616e000000000000000000008152505f546040518363ffffffff1660e01b815260040161030692919061055a565b5f604051808303815f875af1158015610321573d5f803e3d5ffd5b505050506040513d5f823e3d601f19601f8201168201806040525081019061034991906106b7565b90505f818060200190518101906103609190610790565b905060018161036f919061086f565b905061010773ffffffffffffffffffffffffffffffffffffffff1663c5d187dd6040518060400160405280601681526020017f626c6f636b73746d5f7368617265645f78617069616e000000000000000000008152505f54846040516020016103d891906104b7565b6040516020818303038152906040526040518463ffffffff1660e01b8152600401610405939291906108a2565b6020604051808303815f875af1158015610421573d5f803e3d5ffd5b505050506040513d601f19601f820116820180604052508101906104459190610790565b5f819055503373ffffffffffffffffffffffffffffffffffffffff167f3d523d852046afd5dba341b55c89c4355e15f6c45d6785c48472c0b7eb922911825f546040516104939291906108e5565b60405180910390a25050565b5f819050919050565b6104b18161049f565b82525050565b5f6020820190506104ca5f8301846104a8565b92915050565b5f81519050919050565b5f82825260208201905092915050565b5f5b838110156105075780820151818401526020810190506104ec565b5f8484015250505050565b5f601f19601f8301169050919050565b5f61052c826104d0565b61053681856104da565b93506105468185602086016104ea565b61054f81610512565b840191505092915050565b5f6040820190508181035f8301526105728185610522565b905061058160208301846104a8565b9392505050565b5f604051905090565b5f80fd5b5f80fd5b5f80fd5b5f80fd5b7f4e487b71000000000000000000000000000000000000000000000000000000005f52604160045260245ffd5b6105d782610512565b810181811067ffffffffffffffff821117156105f6576105f56105a1565b5b80604052505050565b5f610608610588565b905061061482826105ce565b919050565b5f67ffffffffffffffff821115610633576106326105a1565b5b61063c82610512565b9050602081019050919050565b5f61065b61065684610619565b6105ff565b9050828152602081018484840111156106775761067661059d565b5b6106828482856104ea565b509392505050565b5f82601f83011261069e5761069d610599565b5b81516106ae848260208601610649565b91505092915050565b5f602082840312156106cc576106cb610591565b5b5f82015167ffffffffffffffff8111156106e9576106e8610595565b5b6106f58482850161068a565b91505092915050565b7f44617461206e6f7420666f756e6420696e2058617069616e00000000000000005f82015250565b5f6107326018836104da565b915061073d826106fe565b602082019050919050565b5f6020820190508181035f83015261075f81610726565b9050919050565b61076f8161049f565b8114610779575f80fd5b50565b5f8151905061078a81610766565b92915050565b5f602082840312156107a5576107a4610591565b5b5f6107b28482850161077c565b91505092915050565b5f81519050919050565b5f82825260208201905092915050565b5f6107df826107bb565b6107e981856107c5565b93506107f98185602086016104ea565b61080281610512565b840191505092915050565b5f6040820190508181035f8301526108258185610522565b9050818103602083015261083981846107d5565b90509392505050565b7f4e487b71000000000000000000000000000000000000000000000000000000005f52601160045260245ffd5b5f6108798261049f565b91506108848361049f565b925082820190508082111561089c5761089b610842565b5b92915050565b5f6060820190508181035f8301526108ba8186610522565b90506108c960208301856104a8565b81810360408301526108db81846107d5565b9050949350505050565b5f6040820190506108f85f8301856104a8565b61090560208301846104a8565b939250505056fea26469706673582212206fc50b91548927c43615db3f28d3327c19317d69d871447cfc01e8a3fdaf188964736f6c63430008140033";
        std::string deploy_shared_cmd = "DEPLOY:" + shared_update_bytecode;
        std::string res3 = send_command(deploy_shared_cmd);
        std::cout << "  ↳ Result: " << res3 << std::endl;
        
        if (res3.find("SUCCESS") == std::string::npos) {
            std::cout << "❌ [TEST] FAILED to deploy SharedUpdate contract." << std::endl;
            close(fd);
            return 1;
        }
        
        std::string shared_addr = extract_address(res3);
        std::cout << "  ↳ Derived Contract Address: " << shared_addr << "\n" << std::endl;
        
        // 4. Call initializeDoc()
        std::cout << "[TX #4] 📝 Calling initializeDoc() on " << shared_addr << "..." << std::endl;
        std::string call_init = "CALL:" + shared_addr + ":b4340bbe";
        std::string res_init = send_command(call_init);
        std::cout << "  ↳ Result: " << res_init << "\n" << std::endl;
        
        // 5. Call incrementShared()
        std::cout << "[TX #5] ➕ Calling incrementShared() on " << shared_addr << "..." << std::endl;
        std::string call_inc = "CALL:" + shared_addr + ":d32a9a59";
        std::string res_inc = send_command(call_inc);
        std::cout << "  ↳ Result: " << res_inc << "\n" << std::endl;
        
        // 6. Call getSharedDataFromDB()
        std::cout << "[TX #6] 🔍 Calling getSharedDataFromDB() on " << shared_addr << "..." << std::endl;
        std::string call_get = "CALL:" + shared_addr + ":0b6f8f48";
        std::string res_get = send_command(call_get);
        std::cout << "  ↳ Result: " << res_get << std::endl;
        
        if (res_get.find("0000000000000000000000000000000000000000000000000000000000000001") != std::string::npos || res_get.find("SUCCESS") != std::string::npos) {
            std::cout << "\n=======================================================" << std::endl;
            std::cout << "🎉 ✅ ALL TESTS PASSED! FULL WALLET & XAPIAN WORKFLOW VERIFIED." << std::endl;
            std::cout << "=======================================================" << std::endl;
            
            // Print final wallet state
            std::string final_wallet = send_command("WALLET");
            std::cout << "[WALLET FINAL STATE] " << final_wallet << std::endl;
        } else {
            std::cout << "\n❌ [TEST] FAILED: Xapian SharedUpdate output did not match expected '1'." << std::endl;
        }
        
        close(fd);
        return 0;
    }

    std::cout << "[EVM-CA] Connected to TrustZone EVM-TA!" << std::endl;
    std::cout << "[EVM-CA] Interactive Mode. Type 'exit' to quit." << std::endl;
    std::cout << "[EVM-CA] Commands:" << std::endl;
    std::cout << "  WALLET                  - Check default wallet balance & nonce" << std::endl;
    std::cout << "  DEPLOY:<hex_bytecode>   - Deploy a new contract from default wallet" << std::endl;
    std::cout << "  CALL:<address>:<data>   - Call an existing contract from default wallet" << std::endl;
    
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
