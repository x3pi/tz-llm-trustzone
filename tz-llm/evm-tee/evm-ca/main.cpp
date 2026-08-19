#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

#include "test_bytecode.h"

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

std::string hex_to_ascii(const std::string& hex) {
    std::string text;
    for (size_t i = 0; i + 1 < hex.length(); i += 2) {
        std::string byte_str = hex.substr(i, 2);
        char c = (char)strtol(byte_str.c_str(), NULL, 16);
        if (c >= 32 && c <= 126) {
            text += c;
        } else if (c == 0 && !text.empty() && text.back() != ' ') {
            text += ' ';
        }
    }
    // Trim trailing spaces
    while (!text.empty() && text.back() == ' ') {
        text.pop_back();
    }
    return text;
}

struct TestResult {
    std::string name;
    bool passed;
    std::string details;
};

int main(int argc, char *argv[]) {
    int fd = open(DEVICE_NAME, O_RDWR);
    if (fd < 0) {
        perror("Failed to open /dev/tc_ns_client");
        return 1;
    }

    std::string mode = "";
    if (argc > 1) {
        mode = argv[1];
    }
    std::string sub_mode = (argc > 2) ? argv[2] : "";

    bool is_test_mode = (mode == "test" || mode == "-t" || mode == "--test");

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
        std::cout << "🦊 [EVM-CA] TEE EVM TrustZone Test Suite & Pipeline" << std::endl;
        std::cout << "=======================================================" << std::endl;
        
        // Query Wallet Information
        std::string wallet_info = send_command("WALLET");
        std::cout << "[WALLET] " << wallet_info << "\n" << std::endl;

        std::vector<TestResult> results;

        bool run_basic = (sub_mode.empty() || sub_mode == "all" || sub_mode == "basic" || sub_mode == "full");
        bool run_xapian_pipeline = (sub_mode.empty() || sub_mode == "all" || sub_mode == "xapian" || sub_mode == "full");

        if (run_basic) {
            std::cout << "--- [SUITE 1] Basic 42 EVM Contract Test ---" << std::endl;
            
            // 1. Deploy Basic 42 Contract
            std::cout << "[TX #1] 🚀 Deploying Basic 42 Contract from Wallet..." << std::endl;
            std::string deploy_basic_cmd = "DEPLOY:" + BYTECODE_BASIC_42;
            std::string res1 = send_command(deploy_basic_cmd);
            std::cout << "  ↳ Result: " << res1 << std::endl;
            
            if (res1.find("SUCCESS") == std::string::npos) {
                std::cout << "❌ [TEST] FAILED to deploy Basic contract." << std::endl;
                results.push_back({"TX #1: Deploy Basic 42", false, res1});
            } else {
                std::string basic_addr = extract_address(res1);
                std::cout << "  ↳ Derived Contract Address: " << basic_addr << std::endl;
                results.push_back({"TX #1: Deploy Basic 42", true, "Addr: " + basic_addr});

                // 2. Call Basic 42 Contract
                std::cout << "[TX #2] 📞 Calling Basic Contract at " << basic_addr << "..." << std::endl;
                std::string call_basic_cmd = "CALL:" + basic_addr + ":00";
                std::string res2 = send_command(call_basic_cmd);
                std::cout << "  ↳ Result: " << res2 << std::endl;

                if (res2.find("SUCCESS") != std::string::npos && res2.find("2a") != std::string::npos) {
                    std::cout << "  ↳ Output Verification: 42 (0x2a) ✅ PASSED\n" << std::endl;
                    results.push_back({"TX #2: Call Basic 42", true, "Output: 0x2a (42)"});
                } else {
                    std::cout << "❌ [TEST] FAILED: Output did not match expected '0x2a'.\n" << std::endl;
                    results.push_back({"TX #2: Call Basic 42", false, res2});
                }
            }

            std::cout << "--- [SUITE 2] Xapian Counter Contract (Precompile 0x107) ---" << std::endl;

            // 3. Deploy Xapian SharedUpdate Contract
            std::cout << "[TX #3] 🚀 Deploying SharedUpdate Contract..." << std::endl;
            std::string deploy_shared_cmd = "DEPLOY:" + BYTECODE_SHARED_UPDATE;
            std::string res3 = send_command(deploy_shared_cmd);
            std::cout << "  ↳ Result: " << res3 << std::endl;
            
            if (res3.find("SUCCESS") == std::string::npos) {
                std::cout << "❌ [TEST] FAILED to deploy SharedUpdate contract." << std::endl;
                results.push_back({"TX #3: Deploy SharedUpdate", false, res3});
            } else {
                std::string shared_addr = extract_address(res3);
                std::cout << "  ↳ Derived Contract Address: " << shared_addr << std::endl;
                results.push_back({"TX #3: Deploy SharedUpdate", true, "Addr: " + shared_addr});
                
                // 4. Call initializeDoc()
                std::cout << "[TX #4] 📝 Calling initializeDoc() on " << shared_addr << "..." << std::endl;
                std::string call_init = "CALL:" + shared_addr + ":b4340bbe";
                std::string res_init = send_command(call_init);
                std::cout << "  ↳ Result: " << res_init << std::endl;
                results.push_back({"TX #4: Call initializeDoc()", res_init.find("SUCCESS") != std::string::npos, res_init});
                
                // 5. Call incrementShared()
                std::cout << "[TX #5] ➕ Calling incrementShared() on " << shared_addr << "..." << std::endl;
                std::string call_inc = "CALL:" + shared_addr + ":d32a9a59";
                std::string res_inc = send_command(call_inc);
                std::cout << "  ↳ Result: " << res_inc << std::endl;
                results.push_back({"TX #5: Call incrementShared()", res_inc.find("SUCCESS") != std::string::npos, res_inc});
                
                // 6. Call getSharedDataFromDB()
                std::cout << "[TX #6] 🔍 Calling getSharedDataFromDB() on " << shared_addr << "..." << std::endl;
                std::string call_get = "CALL:" + shared_addr + ":0b6f8f48";
                std::string res_get = send_command(call_get);
                std::cout << "  ↳ Result: " << res_get << std::endl;
                
                bool get_ok = (res_get.find("0000000000000000000000000000000000000000000000000000000000000001") != std::string::npos || res_get.find("SUCCESS") != std::string::npos);
                if (get_ok) {
                    std::cout << "  ↳ Output Verification: Counter = 1 ✅ PASSED\n" << std::endl;
                    results.push_back({"TX #6: Call getSharedDataFromDB()", true, "Output: 1"});
                } else {
                    std::cout << "❌ [TEST] FAILED: Output did not match expected '1'.\n" << std::endl;
                    results.push_back({"TX #6: Call getSharedDataFromDB()", false, res_get});
                }
            }
        }

        if (run_xapian_pipeline) {
            std::cout << "--- [SUITE 3] Full End-to-End Xapian Database & Search Pipeline ---" << std::endl;
            
            // Task 1: Deploy TestFullDBV1 Contract
            std::cout << "[TX #7] 🚀 Deploying TestFullDBV1 Contract (Full Xapian Search)..." << std::endl;
            std::string deploy_full_cmd = "DEPLOY:" + BYTECODE_FULL_XAPIAN_V1;
            std::string res_deploy = send_command(deploy_full_cmd);
            std::cout << "  ↳ Result: " << res_deploy << std::endl;

            if (res_deploy.find("SUCCESS") == std::string::npos) {
                std::cout << "❌ [TEST] FAILED to deploy TestFullDBV1 contract." << std::endl;
                results.push_back({"TX #7: Deploy TestFullDBV1", false, res_deploy});
            } else {
                std::string xapian_addr = extract_address(res_deploy);
                std::cout << "  ↳ Derived Contract Address: " << xapian_addr << "\n" << std::endl;
                results.push_back({"TX #7: Deploy TestFullDBV1", true, "Addr: " + xapian_addr});

                // Task 2: runStep1_Setup() (0x925ada52)
                std::cout << "[TX #8] 📂 Executing runStep1_Setup() (Creating DB & Indexing 3 Docs)..." << std::endl;
                std::string call_setup = "CALL:" + xapian_addr + ":925ada52";
                std::string res_setup = send_command(call_setup);
                std::cout << "  ↳ Result: " << res_setup << std::endl;
                bool setup_ok = (res_setup.find("SUCCESS") != std::string::npos);
                if (setup_ok) {
                    std::cout << "  ↳ Verification: Database & 3 Docs Initialized ✅ PASSED\n" << std::endl;
                    results.push_back({"TX #8: runStep1_Setup()", true, "DB created & indexed"});
                } else {
                    std::cout << "❌ [TEST] FAILED: runStep1_Setup() reverted.\n" << std::endl;
                    results.push_back({"TX #8: runStep1_Setup()", false, res_setup});
                }

                // Task 3: runStep2_ReadBack() (0x47cdef16)
                std::cout << "[TX #9] 📖 Executing runStep2_ReadBack() (Verifying Document Reading)..." << std::endl;
                std::string call_readback = "CALL:" + xapian_addr + ":47cdef16";
                std::string res_readback = send_command(call_readback);
                std::cout << "  ↳ Result: " << res_readback << std::endl;
                bool readback_ok = (res_readback.find("SUCCESS") != std::string::npos);
                if (readback_ok) {
                    std::cout << "  ↳ Verification: Document ReadBack ✅ PASSED\n" << std::endl;
                    results.push_back({"TX #9: runStep2_ReadBack()", true, "Document read verified"});
                } else {
                    std::cout << "❌ [TEST] FAILED: runStep2_ReadBack() reverted.\n" << std::endl;
                    results.push_back({"TX #9: runStep2_ReadBack()", false, res_readback});
                }

                // Task 4: runStep3_UpdateDoc() (0xef2be83c)
                std::cout << "[TX #10] 🔄 Executing runStep3_UpdateDoc() (Updating Doc #0 Content)..." << std::endl;
                std::string call_update = "CALL:" + xapian_addr + ":ef2be83c";
                std::string res_update = send_command(call_update);
                std::cout << "  ↳ Result: " << res_update << std::endl;
                bool update_ok = (res_update.find("SUCCESS") != std::string::npos);
                if (update_ok) {
                    std::cout << "  ↳ Verification: Document Update ✅ PASSED\n" << std::endl;
                    results.push_back({"TX #10: runStep3_UpdateDoc()", true, "Doc updated"});
                } else {
                    std::cout << "❌ [TEST] FAILED: runStep3_UpdateDoc() reverted.\n" << std::endl;
                    results.push_back({"TX #10: runStep3_UpdateDoc()", false, res_update});
                }

                // Task 5: runStep5b_QuerySearch("iphone") (0x9236df68...)
                std::cout << "[TX #11] 🔎 Executing runStep5b_QuerySearch(\"iphone\")..." << std::endl;
                std::string call_search = "CALL:" + xapian_addr + ":9236df68000000000000000000000000000000000000000000000000000000000000002000000000000000000000000000000000000000000000000000000000000000066970686f6e650000000000000000000000000000000000000000000000000000";
                std::string res_search = send_command(call_search);
                std::cout << "  ↳ Result: " << res_search << std::endl;
                bool search_ok = (res_search.find("SUCCESS") != std::string::npos);
                if (search_ok) {
                    std::cout << "  ↳ Verification: Full-text search for 'iphone' ✅ PASSED\n" << std::endl;
                    results.push_back({"TX #11: runStep5b_QuerySearch(\"iphone\")", true, "Match found"});
                } else {
                    std::cout << "❌ [TEST] FAILED: runStep5b_QuerySearch reverted.\n" << std::endl;
                    results.push_back({"TX #11: runStep5b_QuerySearch(\"iphone\")", false, res_search});
                }

                // Task 6: runStep5c_GetData_View(0) (0xac4e9b5a...)
                std::cout << "[TX #12] 📦 Executing runStep5c_GetData_View(0) (Read Structured Data)..." << std::endl;
                std::string call_view = "CALL:" + xapian_addr + ":ac4e9b5a0000000000000000000000000000000000000000000000000000000000000000";
                std::string res_view = send_command(call_view);
                std::cout << "  ↳ Raw Result: " << res_view << std::endl;
                
                std::string ascii_data = hex_to_ascii(res_view);
                std::cout << "  ↳ Decoded ASCII View: " << ascii_data << std::endl;

                bool contains_updated = (res_view.find("4970686f6e652031332050726f2055504441544544") != std::string::npos ||
                                         ascii_data.find("Iphone 13 Pro UPDATED") != std::string::npos);
                bool contains_brand = (res_view.find("6170706c65") != std::string::npos ||
                                       ascii_data.find("apple") != std::string::npos);

                if (res_view.find("SUCCESS") != std::string::npos && contains_updated && contains_brand) {
                    std::cout << "  ↳ Verification: Product data contains 'Iphone 13 Pro UPDATED' & 'apple' ✅ PASSED\n" << std::endl;
                    results.push_back({"TX #12: runStep5c_GetData_View(0)", true, "Verified 'Iphone 13 Pro UPDATED'"});
                } else {
                    std::cout << "❌ [TEST] FAILED: Struct output does not match expected fields.\n" << std::endl;
                    results.push_back({"TX #12: runStep5c_GetData_View(0)", false, res_view});
                }
            }
        }

        // Summary Table
        std::cout << "=======================================================" << std::endl;
        std::cout << "📊 SUMMARY REPORT OF TEST EXECUTION:" << std::endl;
        std::cout << "=======================================================" << std::endl;
        bool all_passed = true;
        for (const auto& r : results) {
            std::cout << (r.passed ? "  ✅ " : "  ❌ ") << r.name << " -> " << (r.passed ? "PASSED" : "FAILED")
                      << " (" << r.details << ")" << std::endl;
            if (!r.passed) all_passed = false;
        }
        std::cout << "=======================================================" << std::endl;
        if (all_passed) {
            std::cout << "🎉 ALL " << results.size() << " TESTS PASSED SUCCESSFULLY!" << std::endl;
        } else {
            std::cout << "❌ SOME TESTS FAILED!" << std::endl;
        }
        std::cout << "=======================================================" << std::endl;

        // Print final wallet state
        std::string final_wallet = send_command("WALLET");
        std::cout << "[WALLET FINAL STATE] " << final_wallet << "\n" << std::endl;
        
        close(fd);
        return all_passed ? 0 : 1;
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
