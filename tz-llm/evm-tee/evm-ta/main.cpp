#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <sstream>
#include "mvm/processor.h"
#include "my_global_state.h"
#include "my_extension/my_extension.h"
#include "my_logger.h"
#include "mvm/account.h"
#include "mvm/transaction.h"
#include "mvm/log.h"

namespace mvm {
bool handle_cross_chain_precompile(
    GlobalState &gs,
    const std::vector<uint8_t> &input,
    std::vector<uint8_t> &output,
    AccountState &acc,
    const uint256_t value,
    LogHandler &log_handler,
    const uint256_t timestamp,
    const Address addr) {
    return true; // Dummy
}
}

// ChCore SMC structures
typedef int cap_t;

extern "C" {
    struct smc_registers {
        unsigned long x0;
        unsigned long x1;
        unsigned long x2;
        unsigned long x3;
        unsigned long x4;
    };
    unsigned long usys_tee_wait_switch_req(struct smc_registers *regs);
    unsigned long usys_tee_switch_req(struct smc_registers *regs);
    int usys_tee_create_ns_pmo(unsigned long paddr, unsigned long size);
    void *chcore_auto_map_pmo(int pmo_cap, unsigned long size, unsigned long perm);
}

#define VMR_READ (1 << 0)
#define VMR_WRITE (1 << 1)
#define SHM_SIZE (1 * 1024 * 1024)

// Helpers to parse hex
std::vector<uint8_t> hex_to_bytes(const std::string& hex) {
    std::vector<uint8_t> bytes;
    for (unsigned int i = 0; i < hex.length(); i += 2) {
        std::string byteString = hex.substr(i, 2);
        uint8_t byte = (uint8_t) strtol(byteString.c_str(), NULL, 16);
        bytes.push_back(byte);
    }
    return bytes;
}

std::string bytes_to_hex(const std::vector<uint8_t>& bytes) {
    std::string hex;
    char buf[3];
    for (uint8_t b : bytes) {
        sprintf(buf, "%02x", b);
        hex += buf;
    }
    return hex;
}

int main() {
    std::cout << "[EVM-TA] Starting inside TEE! (RAM-only EVM Mode - Version 4.0)" << std::endl;

    // Initialize EVM environment
    mvm::BlockContext bc;
    mvm::MyGlobalState gs(bc, true);
    mvm::VectorLogHandler log_handler;
    MyExtension extension(nullptr, true, nullptr);
    MyLogger native_logger;

    mvm::Processor processor(gs, log_handler, extension, native_logger);

    struct smc_registers req = {0};
    std::cout << "[EVM-TA] Waiting for SMC from Normal World..." << std::endl;
    unsigned long paddr = usys_tee_wait_switch_req(&req);
    
    // TEE os kernel (tzdriver in normal world) will yield an SMC to wake us up during init
    // Ignore all dummy SMCs until we get a real paddr
    while (paddr == 0) {
        std::cout << "[EVM-TA] Got initialization SMC (paddr=0), waiting for the real one..." << std::endl;
        
        // We MUST yield back to Normal World to acknowledge the dummy SMC, 
        // otherwise wait_switch_req will return immediately!
        req.x1 = 0;
        req.x2 = 0;
        usys_tee_switch_req(&req);

        // Now wait for the next SMC
        paddr = usys_tee_wait_switch_req(&req);
    }

        std::cout << "[EVM-TA] Received SMC! paddr: " << std::hex << paddr << std::dec << std::endl;

    cap_t pmo = usys_tee_create_ns_pmo(paddr, SHM_SIZE);
    void* vaddr = chcore_auto_map_pmo(pmo, SHM_SIZE, VMR_READ | VMR_WRITE);

    if (vaddr) {
        char* shm_buf = (char*)vaddr;
        sprintf(shm_buf, "msg from tee\n");

        std::cout << "[EVM-TA] Initialization complete. Entering service loop..." << std::endl;

        while (1) {
            std::cout << "[EVM-TA] Yielding and waiting for query..." << std::endl;
            usys_tee_wait_switch_req(&req);

            std::string input(shm_buf);
            
            try {
                if (input.rfind("DEPLOY:", 0) == 0) {
                    std::string hex_data = input.substr(7);
                    if (hex_data.rfind("0x", 0) == 0) hex_data = hex_data.substr(2);
                    
                    std::vector<uint8_t> bytecode = hex_to_bytes(hex_data);
                    
                    mvm::Address caller = 0; // Dummy caller
                    mvm::Address callee = 1; // Dummy contract address
                    mvm::Transaction tx(caller, 0, 1, 10000000, 0, false);
                    
                    // Prepare Callee Account
                    gs.create(callee, 0, {}, 0);
                    auto callee_state = gs.get(callee);
                    
                    // Run deployment
                    mvm::ExecResult res = processor.run(tx, true, caller, callee_state, bytecode, 0, nullptr, false);
                    
                    if (res.er == mvm::ExitReason::returned) {
                        // The output of a deploy tx is the runtime bytecode
                        callee_state.acc.set_code(std::move(res.output));
                        std::string out_msg = "SUCCESS - Contract deployed at address 1. Gas used: " + std::to_string(res.gas_used);
                        strncpy(shm_buf, out_msg.c_str(), SHM_SIZE - 1);
                    } else {
                        std::string err_msg = "FAILED to deploy. Exception: " + res.exmsg;
                        strncpy(shm_buf, err_msg.c_str(), SHM_SIZE - 1);
                    }
                    
                } else if (input.rfind("CALL:", 0) == 0) {
                    // CALL:<address_int>:<hex_data>
                    size_t pos1 = input.find(':', 5);
                    if (pos1 != std::string::npos) {
                        int addr_int = std::stoi(input.substr(5, pos1 - 5));
                        std::string hex_data = input.substr(pos1 + 1);
                        if (hex_data.rfind("0x", 0) == 0) hex_data = hex_data.substr(2);
                        
                        mvm::Address callee = addr_int;
                        auto callee_state = gs.get(callee);
                        std::vector<uint8_t> calldata = hex_to_bytes(hex_data);
                        
                        mvm::Address caller = 0;
                        mvm::Transaction tx(caller, 0, 1, 10000000, 0, false);
                        
                        mvm::ExecResult res = processor.run(tx, false, caller, callee_state, calldata, 0, nullptr, false);
                        
                        if (res.er == mvm::ExitReason::returned) {
                            std::string out_msg = "SUCCESS - Output: " + bytes_to_hex(res.output) + " Gas: " + std::to_string(res.gas_used);
                            strncpy(shm_buf, out_msg.c_str(), SHM_SIZE - 1);
                        } else {
                            std::string err_msg = "FAILED execution. Exception: " + res.exmsg;
                            strncpy(shm_buf, err_msg.c_str(), SHM_SIZE - 1);
                        }
                    } else {
                        strncpy(shm_buf, "ERROR: Invalid CALL format", SHM_SIZE - 1);
                    }
                } else {
                    strncpy(shm_buf, "ERROR: Unknown command. Use DEPLOY: or CALL:", SHM_SIZE - 1);
                }
            } catch (const std::exception& e) {
                std::string err = "EXCEPTION: " + std::string(e.what());
                strncpy(shm_buf, err.c_str(), SHM_SIZE - 1);
            }

            // SMC back to Linux to signal completion
            req.x1 = 0;
            req.x2 = 0;
            usys_tee_switch_req(&req);
        }
    } else {
        std::cout << "[EVM-TA] Failed to map SHM!" << std::endl;
    }

    while (1) {}
    return 0;
}
