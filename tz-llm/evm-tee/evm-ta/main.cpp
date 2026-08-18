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
#include "mvm/util.h"
#include "state.h"
#include "xapian/xapian_manager.h"
#include "xapian/xapian_registry.h"

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

mvm::Address parse_address(const std::string& str) {
    if (str.rfind("0x", 0) == 0 || str.rfind("0X", 0) == 0) {
        return mvm::to_uint256(str);
    }
    bool all_digits = true;
    for (char c : str) {
        if (!isdigit(c)) { all_digits = false; break; }
    }
    if (all_digits && str.length() < 10) {
        return mvm::Address(std::stoull(str));
    }
    return mvm::to_uint256("0x" + str);
}

int main() {
    std::cout << "[EVM-TA] Starting inside TEE! (Standard Single-Wallet EVM Mode - Version 5.0)" << std::endl;

    // Standard Ethereum EOA Test Wallet: 0xf39Fd6e51aad88F6F4ce6aB8827279cffFb92266
    const mvm::Address DEFAULT_WALLET = mvm::to_uint256("0xf39Fd6e51aad88F6F4ce6aB8827279cffFb92266");

    // Initialize EVM environment
    mvm::BlockContext bc;
    mvm::MyGlobalState gs(bc, true);
    mvm::VectorLogHandler log_handler;
    MyExtension extension(nullptr, false, nullptr);
    MyLogger native_logger;

    mvm::Processor processor(gs, log_handler, extension, native_logger);

    // Initialize Default Wallet with 1,000,000 ETH (10^24 Wei) and Nonce = 0
    uint256_t initial_balance = intx::from_string<uint256_t>("1000000000000000000000000");
    gs.create(DEFAULT_WALLET, initial_balance, {}, 0);
    auto wallet_state = State::getInstance(DEFAULT_WALLET);
    wallet_state->setBalance(initial_balance);
    wallet_state->setNonce(0);

    static uint64_t tx_nonce = 1;

    struct smc_registers req = {0};
    std::cout << "[EVM-TA] Initialized Wallet: " << mvm::address_to_hex_string(DEFAULT_WALLET) << std::endl;
    std::cout << "[EVM-TA] Waiting for SMC from Normal World..." << std::endl;
    unsigned long paddr = usys_tee_wait_switch_req(&req);
    
    while (paddr == 0) {
        std::cout << "[EVM-TA] Got initialization SMC (paddr=0), waiting for the real one..." << std::endl;
        req.x1 = 0;
        req.x2 = 0;
        usys_tee_switch_req(&req);
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
            std::cout << "[EVM-TA] Received command: " << input << std::endl;

            try {
                if (input.rfind("DEPLOY:", 0) == 0) {
                    mvm::Address caller = DEFAULT_WALLET;
                    std::string hex_data;
                    
                    // Support optional DEPLOY:<caller>:<bytecode> or standard DEPLOY:<bytecode>
                    size_t pos1 = input.find(':', 7);
                    if (pos1 != std::string::npos && pos1 < 50) {
                        caller = parse_address(input.substr(7, pos1 - 7));
                        hex_data = input.substr(pos1 + 1);
                    } else {
                        hex_data = input.substr(7);
                    }

                    if (hex_data.rfind("0x", 0) == 0 || hex_data.rfind("0X", 0) == 0) hex_data = hex_data.substr(2);
                    std::vector<uint8_t> bytecode = hex_to_bytes(hex_data);
                    
                    auto caller_state = gs.get(caller);
                    uint64_t nonce = static_cast<uint64_t>(caller_state.acc.get_nonce());
                    
                    // Standard Ethereum contract address generation: keccak256(rlp([sender, nonce]))
                    mvm::Address callee = mvm::generate_address(caller, nonce);
                    caller_state.acc.set_nonce(nonce + 1);
                    
                    mvm::Transaction tx(caller, nonce, 1, 10000000, 0, false);
                    uint256_t current_tx_hash = intx::uint256(tx_nonce++);
                    extension.txHash = &current_tx_hash;
                    
                    // Prepare Callee Account
                    gs.create(callee, 0, {}, 0);
                    auto callee_state = gs.get(callee);
                    
                    // Run deployment
                    mvm::ExecResult res = processor.run(tx, true, caller, callee_state, bytecode, 0, nullptr, false);
                    std::cout << "[EVM-TA] DEPLOY result: er=" << (int)res.er << " exmsg=" << res.exmsg << std::endl;
                    
                    if (res.er == mvm::ExitReason::returned) {
                        auto callee_inst = State::getInstance(callee);
                        callee_inst->setCode(res.output);
                        callee_inst->setBalance(callee_state.acc.get_balance());
                        callee_inst->setNonce(callee_state.acc.get_nonce());

                        callee_state.acc.set_code(std::move(res.output));

                        auto caller_inst = State::getInstance(caller);
                        caller_inst->setBalance(caller_state.acc.get_balance());
                        caller_inst->setNonce(caller_state.acc.get_nonce());

                        // Commit storage changes
                        gs.iterate_storage_changes([&](const mvm::Address &a, const uint256_t &k, const uint256_t &v) {
                            uint8_t b_key[32];
                            mvm::to_big_endian(k, b_key);
                            State::getInstance(a)->insertOrUpdate(State::toKeyType(b_key), v);
                        });
                        gs.clear_differences();

                        // Commit Xapian Tx Buffer and DB
                        registry.commitBufferForTxHash(&current_tx_hash);
                        XapianManager::commitAllInstances();

                        std::string out_msg = "SUCCESS - Contract deployed at address " + mvm::address_to_hex_string(callee) +
                                              " by " + mvm::address_to_hex_string(caller) +
                                              " (Nonce: " + std::to_string(nonce) + "). Gas used: " + std::to_string(res.gas_used);
                        strncpy(shm_buf, out_msg.c_str(), SHM_SIZE - 1);
                    } else {
                        std::string err_msg = "FAILED to deploy. Exception: " + res.exmsg;
                        strncpy(shm_buf, err_msg.c_str(), SHM_SIZE - 1);
                    }
                    
                } else if (input.rfind("CALL:", 0) == 0) {
                    // Support CALL:<target>:<hex_data> or CALL:<caller>:<target>:<hex_data>
                    size_t pos1 = input.find(':', 5);
                    if (pos1 != std::string::npos) {
                        mvm::Address caller = DEFAULT_WALLET;
                        mvm::Address callee;
                        std::string hex_data;

                        size_t pos2 = input.find(':', pos1 + 1);
                        if (pos2 != std::string::npos) {
                            caller = parse_address(input.substr(5, pos1 - 5));
                            callee = parse_address(input.substr(pos1 + 1, pos2 - pos1 - 1));
                            hex_data = input.substr(pos2 + 1);
                        } else {
                            callee = parse_address(input.substr(5, pos1 - 5));
                            hex_data = input.substr(pos1 + 1);
                        }

                        if (hex_data.rfind("0x", 0) == 0 || hex_data.rfind("0X", 0) == 0) hex_data = hex_data.substr(2);
                        std::vector<uint8_t> calldata = hex_to_bytes(hex_data);
                        
                        auto caller_state = gs.get(caller);
                        uint64_t nonce = static_cast<uint64_t>(caller_state.acc.get_nonce());
                        caller_state.acc.set_nonce(nonce + 1);

                        mvm::Transaction tx(caller, nonce, 1, 10000000, 0, false);
                        uint256_t current_tx_hash = intx::uint256(tx_nonce++);
                        extension.txHash = &current_tx_hash;

                        auto callee_state = gs.get(callee);
                        mvm::ExecResult res = processor.run(tx, false, caller, callee_state, calldata, 0, nullptr, false);
                        std::cout << "[EVM-TA] CALL callee=" << mvm::address_to_hex_string(callee) 
                                  << " calldata_len=" << calldata.size() 
                                  << " res.er=" << (int)res.er << " exmsg=" << res.exmsg << std::endl;
                        
                        if (res.er == mvm::ExitReason::returned) {
                            auto callee_inst = State::getInstance(callee);
                            callee_inst->setBalance(callee_state.acc.get_balance());
                            callee_inst->setNonce(callee_state.acc.get_nonce());

                            auto caller_inst = State::getInstance(caller);
                            caller_inst->setBalance(caller_state.acc.get_balance());
                            caller_inst->setNonce(caller_state.acc.get_nonce());

                            gs.iterate_storage_changes([&](const mvm::Address &a, const uint256_t &k, const uint256_t &v) {
                                uint8_t b_key[32];
                                mvm::to_big_endian(k, b_key);
                                State::getInstance(a)->insertOrUpdate(State::toKeyType(b_key), v);
                            });
                            gs.clear_differences();

                            // Commit Xapian Tx Buffer and DB
                            registry.commitBufferForTxHash(&current_tx_hash);
                            XapianManager::commitAllInstances();

                            std::string out_msg = "SUCCESS - Output: " + bytes_to_hex(res.output) +
                                                  " Gas: " + std::to_string(res.gas_used) +
                                                  " Nonce: " + std::to_string(nonce);
                            strncpy(shm_buf, out_msg.c_str(), SHM_SIZE - 1);
                        } else {
                            std::string err_msg = "FAILED execution. Exception: " + res.exmsg;
                            strncpy(shm_buf, err_msg.c_str(), SHM_SIZE - 1);
                        }
                    } else {
                        strncpy(shm_buf, "ERROR: Invalid CALL format", SHM_SIZE - 1);
                    }
                } else if (input.rfind("GET_WALLET:", 0) == 0 || input == "WALLET") {
                    mvm::Address target = DEFAULT_WALLET;
                    if (input.rfind("GET_WALLET:", 0) == 0) {
                        target = parse_address(input.substr(11));
                    }
                    if (gs.exists(target)) {
                        auto acc = gs.get(target);
                        std::string out_msg = "WALLET: " + mvm::address_to_hex_string(target) +
                                              " BALANCE: " + mvm::to_hex_string(acc.acc.get_balance()) +
                                              " NONCE: " + std::to_string((unsigned long long)acc.acc.get_nonce());
                        strncpy(shm_buf, out_msg.c_str(), SHM_SIZE - 1);
                    } else {
                        strncpy(shm_buf, "WALLET: NOT FOUND", SHM_SIZE - 1);
                    }
                } else {
                    strncpy(shm_buf, "ERROR: Unknown command. Use DEPLOY:, CALL:, or GET_WALLET:", SHM_SIZE - 1);
                }
            } catch (const mvm::Exception& e) {
                std::string err = "MVM_EXCEPTION: " + std::string(e.what());
                strncpy(shm_buf, err.c_str(), SHM_SIZE - 1);
            } catch (const std::exception& e) {
                std::string err = "EXCEPTION: " + std::string(e.what());
                strncpy(shm_buf, err.c_str(), SHM_SIZE - 1);
            } catch (...) {
                std::string err = "FATAL: Unknown Exception";
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
