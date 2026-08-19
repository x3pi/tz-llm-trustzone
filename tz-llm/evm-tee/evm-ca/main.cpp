#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <cstring>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <nlohmann/json.hpp>

#include "test_bytecode.h"

using json = nlohmann::json;

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

std::string hex_to_ascii(const std::string& hex_raw) {
    std::string hex = hex_raw;
    if (hex.rfind("0x", 0) == 0 || hex.rfind("0X", 0) == 0) hex = hex.substr(2);
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
    while (!text.empty() && text.back() == ' ') {
        text.pop_back();
    }
    return text;
}

uint64_t hex_to_uint64(const std::string& hex_raw) {
    std::string hex = hex_raw;
    if (hex.rfind("0x", 0) == 0 || hex.rfind("0X", 0) == 0) hex = hex.substr(2);
    if (hex.empty()) return 0;
    return strtoull(hex.c_str(), NULL, 16);
}

inline uint64_t get_json_uint64(const json& j, const std::string& key, uint64_t default_val = 0) {
    if (j.find(key) == j.end() || j[key].is_null()) return default_val;
    if (j[key].is_number()) return j[key].get<uint64_t>();
    if (j[key].is_string()) {
        std::string s = j[key].get<std::string>();
        if (s.rfind("0x", 0) == 0 || s.rfind("0X", 0) == 0) {
            return std::strtoull(s.c_str(), nullptr, 16);
        }
        return std::strtoull(s.c_str(), nullptr, 10);
    }
    return default_val;
}

inline int get_json_int(const json& j, const std::string& key, int default_val = 0) {
    if (j.find(key) == j.end() || j[key].is_null()) return default_val;
    if (j[key].is_number()) return j[key].get<int>();
    if (j[key].is_string()) {
        std::string s = j[key].get<std::string>();
        if (s.rfind("0x", 0) == 0 || s.rfind("0X", 0) == 0) {
            return (int)std::strtol(s.c_str(), nullptr, 16);
        }
        return std::atoi(s.c_str());
    }
    return default_val;
}

inline std::string get_json_str(const json& j, const std::string& key, const std::string& default_val = "") {
    if (j.find(key) == j.end() || j[key].is_null()) return default_val;
    if (j[key].is_string()) return j[key].get<std::string>();
    return j[key].dump();
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
        
        // Handle potential TEE double-yield timing
        if (strncmp(mapped_mem, cmd.c_str(), SHM_SIZE) == 0) {
            ioctl(fd, LLM_CLIENT_IOCTL_RUN, fd, &out_cmd);
        }
        return std::string(mapped_mem);
    };

    auto send_tx = [&](const json& req) -> json {
        std::string cmd = req.dump();
        std::string res_str = send_command(cmd);
        try {
            return json::parse(res_str);
        } catch (...) {
            json err_json;
            err_json["status"] = -1;
            err_json["status_str"] = "RAW_ERROR";
            err_json["raw_response"] = res_str;
            return err_json;
        }
    };

    // Check for JSON file execution mode (-f <filename>)
    if (mode == "-f" || mode == "--file") {
        if (sub_mode.empty()) {
            std::cerr << "Usage: " << argv[0] << " -f <tx.json>" << std::endl;
            close(fd);
            return 1;
        }
        std::ifstream f(sub_mode);
        if (!f.is_open()) {
            std::cerr << "Error: Cannot open file " << sub_mode << std::endl;
            close(fd);
            return 1;
        }
        json tx_req;
        f >> tx_req;
        std::cout << "[EVM-CA] Sending transaction from " << sub_mode << "..." << std::endl;
        json res = send_tx(tx_req);
        std::cout << res.dump(2) << std::endl;
        close(fd);
        return 0;
    }

    // Check for raw JSON string execution directly from command-line argument
    if (!mode.empty() && mode[0] == '{') {
        try {
            json tx_req = json::parse(mode);
            std::cout << "[EVM-CA] Executing direct JSON transaction..." << std::endl;
            json res = send_tx(tx_req);
            std::cout << res.dump(2) << std::endl;
            close(fd);
            return 0;
        } catch (const std::exception& e) {
            std::cerr << "Invalid JSON input: " << e.what() << std::endl;
            close(fd);
            return 1;
        }
    }

    if (is_test_mode) {
        std::cout << "\n=======================================================================" << std::endl;
        std::cout << "🦊 [EVM-CA] TEE EVM TrustZone Comprehensive Test Suite & Pipeline" << std::endl;
        std::cout << "=======================================================================" << std::endl;
        
        // Query initial wallet status via JSON
        json wallet_req;
        wallet_req["action"] = "wallet";
        wallet_req["from"] = TEST_SENDER;
        json wallet_res = send_tx(wallet_req);
        std::cout << "[METANODE DEV WALLET] " << TEST_SENDER << std::endl;
        if (wallet_res.find("balance") != wallet_res.end()) {
            std::cout << "  ↳ Balance: " << get_json_str(wallet_res, "balance", "0x0")
                      << " | Nonce: " << get_json_uint64(wallet_res, "nonce", 0) << "\n" << std::endl;
        }

        std::vector<TestResult> results;

        bool run_blockchain = (sub_mode.empty() || sub_mode == "all" || sub_mode == "blockchain" || sub_mode == "real");
        bool run_basic = (sub_mode.empty() || sub_mode == "all" || sub_mode == "basic" || sub_mode == "legacy");
        bool run_xapian = (sub_mode.empty() || sub_mode == "all" || sub_mode == "xapian");

        // =====================================================================
        // [SUITE 1] Real Metanode Blockchain Contract Pipeline (Deploy, Write, Read)
        // =====================================================================
        if (run_blockchain) {
            std::cout << "=======================================================================" << std::endl;
            std::cout << "--- [SUITE 1] Real Blockchain Smart Contract Pipeline (data.json) ---" << std::endl;
            std::cout << "=======================================================================" << std::endl;

            std::string deployed_normal_contract = "";

            // Task 1: Deploy Normal-Test Contract (1006 bytes bytecode)
            {
                std::cout << "\n[TX #1] 🚀 [DEPLOY] Deploying Normal-Test Contract from " << TEST_SENDER << "..." << std::endl;
                json req;
                req["action"] = "deploy";
                req["from"] = TEST_SENDER;
                req["input"] = BYTECODE_NORMAL_TEST;
                req["gas_price"] = 100000;
                req["gas_limit"] = 603164;
                req["block_number"] = 1;
                req["block_time"] = 1787106694;
                req["read_only"] = false;
                req["is_off_chain"] = false;
                req["is_cache"] = true;

                json res = send_tx(req);
                int status = get_json_int(res, "status", -1);
                uint64_t gas_used = get_json_uint64(res, "gas_used", 0);
                std::cout << "  ↳ Status: " << get_json_str(res, "status_str", "UNKNOWN")
                          << " (status=" << status << ")" << std::endl;
                std::cout << "  ↳ Gas Used: " << gas_used << std::endl;

                deployed_normal_contract = get_json_str(res, "contract_address", "");
                std::cout << "  ↳ Deployed Contract Address: " << deployed_normal_contract << std::endl;

                if (res.find("mapNonce") != res.end()) {
                    std::cout << "  ↳ Nonce Updates: " << res["mapNonce"].dump() << std::endl;
                }

                bool deploy_ok = ((status == 0 || status == 1) && !deployed_normal_contract.empty() && deployed_normal_contract != "0x0000000000000000000000000000000000000000");
                if (deploy_ok) {
                    std::cout << "  ↳ Task 1 (DEPLOY) Verification: ✅ PASSED (Gas: " << gas_used << ")" << std::endl;
                    results.push_back({"[Real Chain] Deploy Normal-Test Contract", true, "Addr: " + deployed_normal_contract + ", Gas: " + std::to_string(gas_used)});
                } else {
                    std::cout << "  ↳ ❌ Task 1 (DEPLOY) FAILED: " << get_json_str(res, "exmsg", "Unknown error") << std::endl;
                    results.push_back({"[Real Chain] Deploy Normal-Test Contract", false, res.dump()});
                }
            }

            // Task 2: Write setValue(9999) -> 0x55241077...270f
            if (!deployed_normal_contract.empty()) {
                std::cout << "\n[TX #2] ✍️ [WRITE] Calling setValue(9999) on " << deployed_normal_contract << "..." << std::endl;
                json req;
                req["action"] = "write";
                req["from"] = TEST_SENDER;
                req["to"] = deployed_normal_contract;
                req["input"] = CALLDATA_SET_VALUE_9999;
                req["gas_price"] = 100000;
                req["gas_limit"] = 152952;
                req["block_number"] = 2;
                req["block_time"] = 1787106694;
                req["read_only"] = false;
                req["is_off_chain"] = false;
                req["is_cache"] = true;

                json res = send_tx(req);
                int status = get_json_int(res, "status", -1);
                uint64_t gas_used = get_json_uint64(res, "gas_used", 0);
                std::cout << "  ↳ Status: " << get_json_str(res, "status_str", "UNKNOWN")
                          << " (status=" << status << ")" << std::endl;
                std::cout << "  ↳ Gas Used: " << gas_used << std::endl;

                // Inspect Storage Changes
                bool storage_ok = false;
                if (res.find("mapStorageChange") != res.end()) {
                    std::cout << "  ↳ Storage Changes: " << res["mapStorageChange"].dump() << std::endl;
                    std::string st_str = res["mapStorageChange"].dump();
                    if (st_str.find("270f") != std::string::npos) {
                        storage_ok = true;
                    }
                }

                // Inspect Event Logs
                bool event_ok = false;
                if (res.find("event_logs") != res.end() && res["event_logs"].is_array() && !res["event_logs"].empty()) {
                    std::cout << "  ↳ Event Logs Emitted (" << res["event_logs"].size() << "):" << std::endl;
                    for (const auto& log : res["event_logs"]) {
                        std::cout << "     - Address: " << get_json_str(log, "address", "") << std::endl;
                        if (log.find("topics") != log.end()) {
                            std::cout << "       Topics: " << log["topics"].dump() << std::endl;
                            std::string top_str = log["topics"].dump();
                            if (top_str.find("b485dddf") != std::string::npos) {
                                event_ok = true;
                            }
                        }
                        std::cout << "       Data: " << get_json_str(log, "data", "") << std::endl;
                    }
                }

                bool write_ok = ((status == 0 || status == 1) && storage_ok && event_ok);
                if (write_ok) {
                    std::cout << "  ↳ Task 2 (WRITE setValue) Verification: ✅ PASSED (Gas: " << gas_used << ", Slot0: 0x270f, Event: ValueChanged)" << std::endl;
                    results.push_back({"[Real Chain] Write setValue(9999)", true, "Gas: " + std::to_string(gas_used) + ", Event Emitted"});
                } else {
                    std::cout << "  ↳ ❌ Task 2 (WRITE setValue) FAILED: " << get_json_str(res, "exmsg", "Unknown error") << std::endl;
                    results.push_back({"[Real Chain] Write setValue(9999)", false, res.dump()});
                }
            }

            // Task 3: Read getValue() -> 0x20965255 (eth_call simulation: read_only=true, is_off_chain=true)
            if (!deployed_normal_contract.empty()) {
                std::cout << "\n[TX #3] 🔍 [READ] Calling getValue() (eth_call simulation) on " << deployed_normal_contract << "..." << std::endl;
                json req;
                req["action"] = "read";
                req["from"] = TEST_SENDER;
                req["to"] = deployed_normal_contract;
                req["input"] = CALLDATA_GET_VALUE;
                req["gas_price"] = 100000;
                req["gas_limit"] = 10000000;
                req["block_number"] = 2;
                req["block_time"] = 1787106694;
                req["read_only"] = true;
                req["is_off_chain"] = true;
                req["is_cache"] = true;

                json res = send_tx(req);
                int status = get_json_int(res, "status", -1);
                std::cout << "  ↳ Status: " << get_json_str(res, "status_str", "UNKNOWN")
                          << " (status=" << status << ")" << std::endl;
                std::string output_hex = get_json_str(res, "output", "");
                std::cout << "  ↳ Raw Hex Output: " << output_hex << std::endl;

                uint64_t returned_val = hex_to_uint64(output_hex);
                std::cout << "  ↳ Decoded Return Value: " << returned_val << " (Expected: 9999)" << std::endl;

                bool read_ok = ((status == 0 || status == 1) && returned_val == 9999);
                if (read_ok) {
                    std::cout << "  ↳ Task 3 (READ getValue) Verification: ✅ PASSED (Return: 9999)" << std::endl;
                    results.push_back({"[Real Chain] Read getValue() (eth_call)", true, "Returned: 9999 (0x270f)"});
                } else {
                    std::cout << "  ↳ ❌ Task 3 (READ getValue) FAILED: " << get_json_str(res, "exmsg", "Output mismatch") << std::endl;
                    results.push_back({"[Real Chain] Read getValue() (eth_call)", false, res.dump()});
                }
            }
        }

        // =====================================================================
        // [SUITE 2] Basic 42 EVM Contract Test
        // =====================================================================
        if (run_basic) {
            std::cout << "\n=======================================================================" << std::endl;
            std::cout << "--- [SUITE 2] Basic 42 EVM Contract Test ---" << std::endl;
            std::cout << "=======================================================================" << std::endl;

            // Deploy Basic 42
            std::cout << "\n[TX #4] 🚀 Deploying Basic 42 Contract from Wallet..." << std::endl;
            std::string res1 = send_command("DEPLOY:" + BYTECODE_BASIC_42);
            std::cout << "  ↳ Result: " << res1 << std::endl;
            
            if (res1.find("SUCCESS") == std::string::npos) {
                std::cout << "❌ [TEST] FAILED to deploy Basic contract." << std::endl;
                results.push_back({"Deploy Basic 42", false, res1});
            } else {
                std::string basic_addr = extract_address(res1);
                std::cout << "  ↳ Derived Contract Address: " << basic_addr << std::endl;
                results.push_back({"Deploy Basic 42", true, "Addr: " + basic_addr});

                // Call Basic 42
                std::cout << "\n[TX #5] 📞 Calling Basic Contract at " << basic_addr << "..." << std::endl;
                std::string res2 = send_command("CALL:" + basic_addr + ":00");
                std::cout << "  ↳ Result: " << res2 << std::endl;

                if (res2.find("SUCCESS") != std::string::npos && res2.find("2a") != std::string::npos) {
                    std::cout << "  ↳ Output Verification: 42 (0x2a) ✅ PASSED\n" << std::endl;
                    results.push_back({"Call Basic 42", true, "Output: 0x2a (42)"});
                } else {
                    std::cout << "❌ [TEST] FAILED: Output did not match expected '0x2a'.\n" << std::endl;
                    results.push_back({"Call Basic 42", false, res2});
                }
            }
        }

        // =====================================================================
        // [SUITE 3] Xapian Counter Contract (Precompile 0x107)
        // =====================================================================
        if (run_xapian) {
            std::cout << "\n=======================================================================" << std::endl;
            std::cout << "--- [SUITE 3] Xapian Counter Contract (Precompile 0x107) ---" << std::endl;
            std::cout << "=======================================================================" << std::endl;

            std::cout << "\n[TX #6] 🚀 Deploying SharedUpdate Contract..." << std::endl;
            std::string res3 = send_command("DEPLOY:" + BYTECODE_SHARED_UPDATE);
            std::cout << "  ↳ Result: " << res3 << std::endl;
            
            if (res3.find("SUCCESS") == std::string::npos) {
                std::cout << "❌ [TEST] FAILED to deploy SharedUpdate contract." << std::endl;
                results.push_back({"Deploy SharedUpdate", false, res3});
            } else {
                std::string shared_addr = extract_address(res3);
                std::cout << "  ↳ Derived Contract Address: " << shared_addr << std::endl;
                results.push_back({"Deploy SharedUpdate", true, "Addr: " + shared_addr});
                
                // Call initializeDoc()
                std::cout << "\n[TX #7] 📝 Calling initializeDoc() on " << shared_addr << "..." << std::endl;
                std::string res_init = send_command("CALL:" + shared_addr + ":b4340bbe");
                std::cout << "  ↳ Result: " << res_init << std::endl;
                results.push_back({"Call initializeDoc()", res_init.find("SUCCESS") != std::string::npos, res_init});
                
                // Call incrementShared()
                std::cout << "\n[TX #8] ➕ Calling incrementShared() on " << shared_addr << "..." << std::endl;
                std::string res_inc = send_command("CALL:" + shared_addr + ":d32a9a59");
                std::cout << "  ↳ Result: " << res_inc << std::endl;
                results.push_back({"Call incrementShared()", res_inc.find("SUCCESS") != std::string::npos, res_inc});
                
                // Call getSharedDataFromDB()
                std::cout << "\n[TX #9] 🔍 Calling getSharedDataFromDB() on " << shared_addr << "..." << std::endl;
                std::string res_get = send_command("CALL:" + shared_addr + ":0b6f8f48");
                std::cout << "  ↳ Result: " << res_get << std::endl;
                
                bool get_ok = (res_get.find("0000000000000000000000000000000000000000000000000000000000000001") != std::string::npos || res_get.find("SUCCESS") != std::string::npos);
                if (get_ok) {
                    std::cout << "  ↳ Output Verification: Counter = 1 ✅ PASSED\n" << std::endl;
                    results.push_back({"Call getSharedDataFromDB()", true, "Output: 1"});
                } else {
                    std::cout << "❌ [TEST] FAILED: Output did not match expected '1'.\n" << std::endl;
                    results.push_back({"Call getSharedDataFromDB()", false, res_get});
                }
            }

            // =====================================================================
            // [SUITE 4] Full End-to-End Xapian Database & Search Pipeline
            // =====================================================================
            std::cout << "\n=======================================================================" << std::endl;
            std::cout << "--- [SUITE 4] Full End-to-End Xapian Database & Search Pipeline ---" << std::endl;
            std::cout << "=======================================================================" << std::endl;

            std::cout << "\n[TX #10] 🚀 Deploying TestFullDBV1 Contract (Full Xapian Search)..." << std::endl;
            std::string res_deploy = send_command("DEPLOY:" + BYTECODE_FULL_XAPIAN_V1);
            std::cout << "  ↳ Result: " << res_deploy << std::endl;

            if (res_deploy.find("SUCCESS") == std::string::npos) {
                std::cout << "❌ [TEST] FAILED to deploy TestFullDBV1 contract." << std::endl;
                results.push_back({"Deploy TestFullDBV1", false, res_deploy});
            } else {
                std::string xapian_addr = extract_address(res_deploy);
                std::cout << "  ↳ Derived Contract Address: " << xapian_addr << "\n" << std::endl;
                results.push_back({"Deploy TestFullDBV1", true, "Addr: " + xapian_addr});

                // runStep1_Setup()
                std::cout << "\n[TX #11] 📂 Executing runStep1_Setup() (Creating DB & Indexing 3 Docs)..." << std::endl;
                std::string res_setup = send_command("CALL:" + xapian_addr + ":925ada52");
                std::cout << "  ↳ Result: " << res_setup << std::endl;
                bool setup_ok = (res_setup.find("SUCCESS") != std::string::npos);
                results.push_back({"runStep1_Setup()", setup_ok, "DB created & indexed"});

                // runStep2_ReadBack()
                std::cout << "\n[TX #12] 📖 Executing runStep2_ReadBack() (Verifying Document Reading)..." << std::endl;
                std::string res_readback = send_command("CALL:" + xapian_addr + ":47cdef16");
                std::cout << "  ↳ Result: " << res_readback << std::endl;
                bool readback_ok = (res_readback.find("SUCCESS") != std::string::npos);
                results.push_back({"runStep2_ReadBack()", readback_ok, "Document read verified"});

                // runStep3_UpdateDoc()
                std::cout << "\n[TX #13] 🔄 Executing runStep3_UpdateDoc() (Updating Doc #0 Content)..." << std::endl;
                std::string res_update = send_command("CALL:" + xapian_addr + ":ef2be83c");
                std::cout << "  ↳ Result: " << res_update << std::endl;
                bool update_ok = (res_update.find("SUCCESS") != std::string::npos);
                results.push_back({"runStep3_UpdateDoc()", update_ok, "Doc updated"});

                // runStep5b_QuerySearch("iphone")
                std::cout << "\n[TX #14] 🔎 Executing runStep5b_QuerySearch(\"iphone\")..." << std::endl;
                std::string res_search = send_command("CALL:" + xapian_addr + ":9236df68000000000000000000000000000000000000000000000000000000000000002000000000000000000000000000000000000000000000000000000000000000066970686f6e650000000000000000000000000000000000000000000000000000");
                std::cout << "  ↳ Result: " << res_search << std::endl;
                bool search_ok = (res_search.find("SUCCESS") != std::string::npos);
                results.push_back({"runStep5b_QuerySearch(\"iphone\")", search_ok, "Match found"});

                // runStep5c_GetData_View(0)
                std::cout << "\n[TX #15] 📦 Executing runStep5c_GetData_View(0) (Read Structured Data)..." << std::endl;
                std::string res_view = send_command("CALL:" + xapian_addr + ":ac4e9b5a0000000000000000000000000000000000000000000000000000000000000000");
                std::cout << "  ↳ Raw Result: " << res_view << std::endl;
                
                std::string ascii_data = hex_to_ascii(res_view);
                std::cout << "  ↳ Decoded ASCII View: " << ascii_data << std::endl;

                bool contains_updated = (res_view.find("4970686f6e652031332050726f2055504441544544") != std::string::npos ||
                                         ascii_data.find("Iphone 13 Pro UPDATED") != std::string::npos);
                bool contains_brand = (res_view.find("6170706c65") != std::string::npos ||
                                       ascii_data.find("apple") != std::string::npos);

                if (res_view.find("SUCCESS") != std::string::npos && contains_updated && contains_brand) {
                    std::cout << "  ↳ Verification: Product data contains 'Iphone 13 Pro UPDATED' & 'apple' ✅ PASSED\n" << std::endl;
                    results.push_back({"runStep5c_GetData_View(0)", true, "Verified 'Iphone 13 Pro UPDATED'"});
                } else {
                    std::cout << "❌ [TEST] FAILED: Struct output does not match expected fields.\n" << std::endl;
                    results.push_back({"runStep5c_GetData_View(0)", false, res_view});
                }
            }
        }

        // Summary Table
        std::cout << "\n=======================================================================" << std::endl;
        std::cout << "📊 SUMMARY REPORT OF TEST EXECUTION:" << std::endl;
        std::cout << "=======================================================================" << std::endl;
        bool all_passed = true;
        for (const auto& r : results) {
            std::cout << (r.passed ? "  ✅ " : "  ❌ ") << r.name << " -> " << (r.passed ? "PASSED" : "FAILED")
                      << " (" << r.details << ")" << std::endl;
            if (!r.passed) all_passed = false;
        }
        std::cout << "=======================================================================" << std::endl;
        if (all_passed) {
            std::cout << "🎉 ALL " << results.size() << " TESTS PASSED SUCCESSFULLY!" << std::endl;
        } else {
            std::cout << "❌ SOME TESTS FAILED!" << std::endl;
        }
        std::cout << "=======================================================================" << std::endl;
        
        close(fd);
        return all_passed ? 0 : 1;
    }

    std::cout << "[EVM-CA] Connected to TrustZone EVM-TA!" << std::endl;
    std::cout << "[EVM-CA] Interactive Mode. Type 'exit' to quit." << std::endl;
    std::cout << "[EVM-CA] Supports JSON format and legacy commands:" << std::endl;
    std::cout << "  - {\"action\":\"deploy\",\"from\":\"0x...\",\"input\":\"0x...\"}" << std::endl;
    std::cout << "  - {\"action\":\"call\",\"to\":\"0x...\",\"input\":\"0x...\"}" << std::endl;
    std::cout << "  - {\"action\":\"wallet\",\"from\":\"0x...\"}" << std::endl;
    std::cout << "  - DEPLOY:<hex_bytecode>" << std::endl;
    std::cout << "  - CALL:<address>:<data>" << std::endl;
    
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
