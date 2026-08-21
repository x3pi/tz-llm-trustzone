#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <sstream>
#include <nlohmann/json.hpp>
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
    (void)gs; (void)input; (void)output; (void)acc; (void)value; (void)log_handler; (void)timestamp; (void)addr;
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

// Helper string & hex utilities
static inline std::string strip_0x(const std::string& str) {
    if (str.rfind("0x", 0) == 0 || str.rfind("0X", 0) == 0) {
        return str.substr(2);
    }
    return str;
}

static inline std::vector<uint8_t> hex_to_bytes(const std::string& hex_raw) {
    std::string hex = strip_0x(hex_raw);
    if (hex.length() % 2 != 0) {
        hex = "0" + hex;
    }
    std::vector<uint8_t> bytes;
    bytes.reserve(hex.length() / 2);
    for (size_t i = 0; i < hex.length(); i += 2) {
        std::string byteString = hex.substr(i, 2);
        uint8_t byte = (uint8_t) strtol(byteString.c_str(), NULL, 16);
        bytes.push_back(byte);
    }
    return bytes;
}

static inline std::string bytes_to_hex(const std::vector<uint8_t>& bytes) {
    std::string hex;
    hex.reserve(bytes.size() * 2);
    char buf[3];
    for (uint8_t b : bytes) {
        sprintf(buf, "%02x", b);
        hex += buf;
    }
    return hex;
}

static inline mvm::Address parse_address(const std::string& str) {
    if (str.empty()) return mvm::Address(0);
    std::string s = str;
    if (s.rfind("0x", 0) == 0 || s.rfind("0X", 0) == 0) {
        return mvm::to_uint256(s);
    }
    bool all_digits = true;
    for (char c : s) {
        if (!isdigit(c)) { all_digits = false; break; }
    }
    if (all_digits && s.length() < 10) {
        return mvm::Address(std::stoull(s));
    }
    return mvm::to_uint256("0x" + s);
}

static inline uint256_t parse_uint256(const std::string& str) {
    if (str.empty()) return 0;
    if (str.rfind("0x", 0) == 0 || str.rfind("0X", 0) == 0) {
        return intx::from_string<uint256_t>(str);
    }
    bool all_digits = true;
    for (char c : str) {
        if (!isdigit(c)) { all_digits = false; break; }
    }
    if (all_digits) {
        return intx::from_string<uint256_t>(str);
    }
    return intx::from_string<uint256_t>("0x" + str);
}

static inline void init_account(const mvm::Address& addr, const uint256_t& balance, uint64_t nonce) {
    auto inst = State::getInstance(addr);
    inst->setBalance(balance);
    inst->setNonce(nonce);
}

int main() {
    std::cout << "[EVM-TA] Starting inside TEE! (Metanode Full Blockchain EVM Mode - Version 6.0)" << std::endl;

    // Standard Metanode Dev / Test Accounts
    const mvm::Address METANODE_DEV_SENDER = mvm::to_uint256("0x5e582475A504998c5631E12A5a2585D2B1911812");
    const mvm::Address DEFAULT_WALLET      = mvm::to_uint256("0xf39Fd6e51aad88F6F4ce6aB8827279cffFb92266");

    // Dev Accounts from metanode single_chain_data
    const mvm::Address DEV_ACC_1 = mvm::to_uint256("0xc9c0447e21fC7af1A3fC4926828Da31D3319E490");
    const mvm::Address DEV_ACC_2 = mvm::to_uint256("0x73Dc1a2Ce4866D1B12F88209D70ca88B35995E86");
    const mvm::Address DEV_ACC_3 = mvm::to_uint256("0x72D0D4193F5f1f0d33E2De10e6536756edc2be98");
    const mvm::Address DEV_ACC_4 = mvm::to_uint256("0x0EbdB56D8234645e0A50789Ad25D2add441f70cA");
    const mvm::Address DEV_ACC_5 = mvm::to_uint256("0xeAdacD76A17bd3893C59E11A7573175D1c9F69DD");

    // Initialize pre-funded accounts with 1,000,000 ETH (10^24 Wei)
    uint256_t initial_dev_balance = intx::from_string<uint256_t>("1000000000000000000000000");
    
    // Metanode test account starts with nonce = 1 matching blockchain test state
    init_account(METANODE_DEV_SENDER, initial_dev_balance, 1);
    init_account(DEFAULT_WALLET,      initial_dev_balance, 0);
    init_account(DEV_ACC_1,           initial_dev_balance, 0);
    init_account(DEV_ACC_2,           initial_dev_balance, 0);
    init_account(DEV_ACC_3,           initial_dev_balance, 0);
    init_account(DEV_ACC_4,           initial_dev_balance, 0);
    init_account(DEV_ACC_5,           initial_dev_balance, 0);

    static uint64_t tx_counter = 1;

    struct smc_registers req = {0};
    std::cout << "[EVM-TA] Initialized Metanode Dev Wallet: " << mvm::address_to_hex_string(METANODE_DEV_SENDER) 
              << " (Nonce: 1)" << std::endl;
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
            std::cout << "[EVM-TA] Received payload (" << input.length() << " bytes)" << std::endl;

            try {
                // Check if payload is a structured JSON transaction request
                if (!input.empty() && (input.front() == '{' || input.find("{\"") != std::string::npos)) {
                    nlohmann::json j_req = nlohmann::json::parse(input);

                    std::string action = j_req.value("action", "call");
                    std::string from_str = j_req.value("from", "0x5e582475A504998c5631E12A5a2585D2B1911812");
                    std::string to_str = j_req.value("to", "");
                    
                    std::string input_hex = "";
                    if (j_req.find("input") != j_req.end()) input_hex = j_req["input"].get<std::string>();
                    else if (j_req.find("calldata") != j_req.end()) input_hex = j_req["calldata"].get<std::string>();
                    else if (j_req.find("code") != j_req.end()) input_hex = j_req["code"].get<std::string>();
                    else if (j_req.find("constructor") != j_req.end()) input_hex = j_req["constructor"].get<std::string>();

                    std::string amount_str = j_req.value("amount", "0");
                    uint64_t gas_price = j_req.value("gas_price", (uint64_t)100000);
                    uint64_t gas_limit = j_req.value("gas_limit", (uint64_t)10000000);
                    uint64_t block_number = j_req.value("block_number", (uint64_t)1);
                    uint64_t block_time = j_req.value("block_time", (uint64_t)1787106694);
                    uint64_t block_gas_limit = j_req.value("block_gas_limit", (uint64_t)100000000);
                    uint64_t block_base_fee = j_req.value("block_base_fee", (uint64_t)1000);
                    uint64_t block_prevrandao = j_req.value("block_prevrandao", (uint64_t)0);
                    std::string leader_str = j_req.value("leader", "");
                    if (leader_str.empty() && j_req.find("coinbase") != j_req.end()) leader_str = j_req["coinbase"].get<std::string>();
                    std::string mvm_id_str = j_req.value("mvm_id", "0xfd00000000000000000000000000000000000001");
                    
                    // Determine Read-Only vs Write
                    bool read_only = false;
                    if (action == "read" || action == "eth_call" || action == "getReadOnly") {
                        read_only = true;
                    } else if (action == "write" || action == "send") {
                        read_only = false;
                    } else {
                        read_only = j_req.value("read_only", false);
                    }
                    bool is_off_chain = j_req.value("is_off_chain", read_only);
                    bool is_cache = j_req.value("is_cache", true);
                    bool is_debug = j_req.value("is_debug", false);
                    std::string tx_hash_str = j_req.value("tx_hash", "");

                    // Reset State request if specified
                    if (j_req.value("reset_state", false)) {
                        State::clearAllInstances();
                        init_account(METANODE_DEV_SENDER, initial_dev_balance, 1);
                        init_account(DEFAULT_WALLET,      initial_dev_balance, 0);
                        init_account(DEV_ACC_1,           initial_dev_balance, 0);
                    }

                    // Pre-fund any custom init_accounts passed in the request
                    if (j_req.find("init_accounts") != j_req.end() && j_req["init_accounts"].is_array()) {
                        for (const auto& acc_obj : j_req["init_accounts"]) {
                            std::string a_str = acc_obj.value("address", "");
                            std::string b_str = acc_obj.value("balance", "1000000000000000000000000");
                            uint64_t n_val = acc_obj.value("nonce", (uint64_t)0);
                            if (!a_str.empty()) {
                                mvm::Address a_addr = parse_address(a_str);
                                init_account(a_addr, parse_uint256(b_str), n_val);
                                if (acc_obj.find("code") != acc_obj.end()) {
                                    std::string code_hex = acc_obj["code"].get<std::string>();
                                    State::getInstance(a_addr)->setCode(hex_to_bytes(code_hex));
                                }
                            }
                        }
                    }

                    // Query wallet information directly
                    if (action == "wallet" || action == "get_wallet") {
                        mvm::Address target = parse_address(from_str);
                        nlohmann::json j_wallet;
                        j_wallet["status"] = 0;
                        j_wallet["status_str"] = "RETURNED";
                        j_wallet["address"] = mvm::address_to_hex_string(target);
                        if (State::instanceExists(target)) {
                            auto inst = State::getInstance(target);
                            j_wallet["balance"] = mvm::to_hex_string(inst->getBalance());
                            j_wallet["nonce"] = (uint64_t)inst->getNonce();
                        } else {
                            j_wallet["balance"] = "0x0";
                            j_wallet["nonce"] = (uint64_t)0;
                        }
                        std::string out_str = j_wallet.dump();
                        strncpy(shm_buf, out_str.c_str(), SHM_SIZE - 1);
                    } else if (action == "load_storage" || action == "restore_xapian") {
                        int restored_count = 0;
                        if (j_req.find("contracts") != j_req.end() && j_req["contracts"].is_array()) {
                            for (const auto& c_obj : j_req["contracts"]) {
                                std::string a_str = c_obj.value("address", "");
                                if (!a_str.empty()) {
                                    mvm::Address a_addr = parse_address(a_str);
                                    if (!State::instanceExists(a_addr)) {
                                        init_account(a_addr, initial_dev_balance, 0);
                                    }
                                    if (c_obj.find("code") != c_obj.end()) {
                                        std::string code_hex = c_obj["code"].get<std::string>();
                                        State::getInstance(a_addr)->setCode(hex_to_bytes(code_hex));
                                    }
                                }
                            }
                        }
                        if (j_req.find("items") != j_req.end() && j_req["items"].is_array()) {
                            for (const auto& itm : j_req["items"]) {
                                std::string c_str = itm.value("contract", "");
                                std::string d_str = itm.value("dbname", "default");
                                uint64_t v_num = itm.value("version", (uint64_t)1);
                                std::string enc_hex = itm.value("data", "");
                                if (!c_str.empty() && !enc_hex.empty()) {
                                    mvm::Address c_addr = parse_address(c_str);
                                    if (XapianManager::loadDatabaseData(c_addr, d_str, v_num, enc_hex)) {
                                        restored_count++;
                                    }
                                }
                            }
                        }
                        nlohmann::json j_rest;
                        j_rest["status"] = 0;
                        j_rest["status_str"] = "RETURNED";
                        j_rest["restored_count"] = restored_count;
                        std::string out_str = j_rest.dump();
                        strncpy(shm_buf, out_str.c_str(), SHM_SIZE - 1);
                    } else {
                        // Execute transaction (deploy or call)
                        mvm::Address caller = parse_address(from_str);
                        mvm::Address callee = parse_address(to_str);
                        std::vector<uint8_t> calldata = hex_to_bytes(input_hex);
                        uint256_t amount_val = parse_uint256(amount_str);

                        // Ensure caller account exists
                        if (!State::instanceExists(caller)) {
                            init_account(caller, initial_dev_balance, 0);
                        }

                        // Prepare BlockContext
                        uint8_t mvmId_bytes[20] = {0};
                        mvm::Address mvmId_addr = parse_address(mvm_id_str);
                        uint8_t mvm_be[32];
                        mvm::to_big_endian(mvmId_addr, mvm_be);
                        memcpy(mvmId_bytes, mvm_be + 12, 20);

                        uint256_t tx_hash_val = tx_hash_str.empty() ? intx::uint256(tx_counter++) : parse_uint256(tx_hash_str);

                        mvm::BlockContext bc;
                        bc.mvmId = mvmId_bytes;
                        bc.gas_limit = block_gas_limit;
                        bc.time = block_time;
                        bc.base_fee = block_base_fee;
                        bc.number = block_number;
                        bc.prevrandao = block_prevrandao;
                        bc.coinbase = parse_address(leader_str);
                        bc.tx_hash = tx_hash_val;

                        std::vector<mvm::Address> related_addrs;
                        if (j_req.find("related_addresses") != j_req.end() && j_req["related_addresses"].is_array()) {
                            for (const auto& ra : j_req["related_addresses"]) {
                                related_addrs.push_back(parse_address(ra.get<std::string>()));
                            }
                        }

                        mvm::MyGlobalState gs_tx(bc, is_cache, related_addrs);
                        mvm::VectorLogHandler log_handler;
                        MyLogger native_logger;
                        MyExtension extension(bc.mvmId, is_off_chain, &tx_hash_val);
                        mvm::Processor processor(gs_tx, log_handler, extension, native_logger);

                        bool is_deploy = (action == "deploy") || (to_str.empty() && action != "call" && action != "eth_call" && action != "read" && action != "write" && action != "send");
                        mvm::Address deployed_address = mvm::Address(0);

                        mvm::ExecResult res;

                        if (is_deploy) {
                            auto caller_state = gs_tx.get(caller);
                            uint64_t nonce = static_cast<uint64_t>(caller_state.acc.get_nonce());
                            deployed_address = mvm::generate_address(caller, nonce);

                            gs_tx.create(deployed_address, 0, {}, 0);
                            auto contract_state = gs_tx.get(deployed_address);

                            mvm::Transaction tx(caller, amount_val, gas_price, gas_limit, tx_hash_val, is_debug);
                            res = processor.run(tx, true, caller, contract_state, calldata, amount_val, nullptr, false);

                            std::cout << "[EVM-TA] JSON DEPLOY contract=" << mvm::address_to_hex_string(deployed_address)
                                      << " er=" << (int)res.er << " gas=" << res.gas_used << " exmsg=" << res.exmsg << std::endl;

                            if (res.er == mvm::ExitReason::returned) {
                                gs_tx.add_addresses_newly_deploy(deployed_address, res.output);
                                contract_state.acc.set_code(std::move(res.output));

                                if (!read_only) {
                                    // Increment caller nonce
                                    caller_state.acc.set_nonce(nonce + 1);
                                    gs_tx.set_addresses_nonce_change(caller, nonce + 1);
                                    auto caller_inst = State::getInstance(caller);
                                    caller_inst->setNonce(nonce + 1);

                                    // Save deployed contract in State cache
                                    auto contract_inst = State::getInstance(deployed_address);
                                    contract_inst->setCode(contract_state.acc.get_code());
                                    contract_inst->setBalance(contract_state.acc.get_balance());
                                    contract_inst->setNonce(contract_state.acc.get_nonce());

                                    // Commit storage changes
                                    gs_tx.iterate_storage_changes([&](const mvm::Address &a, const uint256_t &k, const uint256_t &v) {
                                        uint8_t b_key[32];
                                        mvm::to_big_endian(k, b_key);
                                        State::getInstance(a)->insertOrUpdate(State::toKeyType(b_key), v);
                                    });

                                    // Commit Xapian Tx Buffer and DB
                                    registry.commitBufferForTxHash(&tx_hash_val);
                                    XapianManager::commitAllInstances();
                                }
                            }
                        } else {
                            auto caller_state = gs_tx.get(caller);
                            auto callee_state = gs_tx.get(callee);

                            if (amount_val > 0) {
                                caller_state.acc.set_balance(caller_state.acc.get_balance() - amount_val);
                                callee_state.acc.set_balance(callee_state.acc.get_balance() + amount_val);
                                gs_tx.add_addresses_sub_balance_change(caller, amount_val);
                                gs_tx.add_addresses_add_balance_change(callee, amount_val);
                            }

                            mvm::Transaction tx(caller, amount_val, gas_price, gas_limit, tx_hash_val, is_debug);
                            res = processor.run(tx, false, caller, callee_state, calldata, amount_val, nullptr, read_only);

                            std::cout << "[EVM-TA] JSON CALL to=" << mvm::address_to_hex_string(callee)
                                      << " readOnly=" << (read_only ? "true" : "false")
                                      << " er=" << (int)res.er << " gas=" << res.gas_used << " exmsg=" << res.exmsg << std::endl;

                            if (!read_only && (res.er == mvm::ExitReason::returned || res.er == mvm::ExitReason::halted)) {
                                uint64_t old_nonce = static_cast<uint64_t>(caller_state.acc.get_nonce());
                                caller_state.acc.set_nonce(old_nonce + 1);
                                gs_tx.set_addresses_nonce_change(caller, old_nonce + 1);
                                auto caller_inst = State::getInstance(caller);
                                caller_inst->setNonce(old_nonce + 1);

                                auto callee_inst = State::getInstance(callee);
                                callee_inst->setBalance(callee_state.acc.get_balance());
                                callee_inst->setNonce(callee_state.acc.get_nonce());

                                // Commit storage changes
                                gs_tx.iterate_storage_changes([&](const mvm::Address &a, const uint256_t &k, const uint256_t &v) {
                                    uint8_t b_key[32];
                                    mvm::to_big_endian(k, b_key);
                                    State::getInstance(a)->insertOrUpdate(State::toKeyType(b_key), v);
                                });

                                // Commit Xapian Tx Buffer and DB
                                registry.commitBufferForTxHash(&tx_hash_val);
                                XapianManager::commitAllInstances();
                            }
                        }

                        // Format full structured JSON response matching Metanode MVMExecuteResult / ExecuteSCResult
                        nlohmann::json j_res;
                        j_res["status"] = static_cast<int>(res.er);
                        switch (res.er) {
                            case mvm::ExitReason::returned: j_res["status_str"] = "RETURNED"; break;
                            case mvm::ExitReason::threw:    j_res["status_str"] = "THREW"; break;
                            case mvm::ExitReason::halted:   j_res["status_str"] = "HALTED"; break;
                            default:                        j_res["status_str"] = "UNKNOWN"; break;
                        }
                        j_res["exception"] = static_cast<int>(res.ex);
                        j_res["exmsg"] = res.exmsg;
                        j_res["output"] = "0x" + bytes_to_hex(res.output);
                        if (is_deploy && res.er == mvm::ExitReason::returned) {
                            j_res["contract_address"] = mvm::address_to_hex_string(deployed_address);
                        } else {
                            j_res["contract_address"] = "";
                        }
                        j_res["gas_used"] = res.gas_used;

                        // Balance maps
                        nlohmann::json j_add_balance = nlohmann::json::object();
                        for (const auto& kv : gs_tx.addresses_add_balance_change) {
                            j_add_balance[mvm::address_to_hex_string(kv.first)] = mvm::to_hex_string(kv.second);
                        }
                        j_res["mapAddBalance"] = j_add_balance;

                        nlohmann::json j_sub_balance = nlohmann::json::object();
                        for (const auto& kv : gs_tx.addresses_sub_balance_change) {
                            j_sub_balance[mvm::address_to_hex_string(kv.first)] = mvm::to_hex_string(kv.second);
                        }
                        j_res["mapSubBalance"] = j_sub_balance;

                        // Nonce map
                        nlohmann::json j_nonce_change = nlohmann::json::object();
                        for (const auto& kv : gs_tx.addresses_nonce_change) {
                            j_nonce_change[mvm::address_to_hex_string(kv.first)] = static_cast<uint64_t>(kv.second);
                        }
                        j_res["mapNonce"] = j_nonce_change;

                        // Code maps
                        nlohmann::json j_code_change = nlohmann::json::object();
                        nlohmann::json j_code_hash = nlohmann::json::object();
                        for (const auto& kv : gs_tx.addresses_newly_deploy) {
                            std::string addr_hex = mvm::address_to_hex_string(kv.first);
                            j_code_change[addr_hex] = "0x" + bytes_to_hex(kv.second);
                            auto h = mvm::keccak_256(kv.second);
                            j_code_hash[addr_hex] = mvm::to_hex_string(h);
                        }
                        j_res["mapCodeChange"] = j_code_change;
                        j_res["mapCodeHash"] = j_code_hash;

                        // Storage changes map: { address: { slot: value } }
                        nlohmann::json j_storage_change = nlohmann::json::object();
                        for (const auto& kv : gs_tx.addresses_storage_change) {
                            std::string addr_hex = mvm::address_to_hex_string(kv.first);
                            nlohmann::json j_slots = nlohmann::json::object();
                            for (const auto& slot_kv : kv.second) {
                                j_slots[mvm::to_hex_string_fixed(slot_kv.first, 64)] = mvm::to_hex_string_fixed(slot_kv.second, 64);
                            }
                            j_storage_change[addr_hex] = j_slots;
                        }
                        j_res["mapStorageChange"] = j_storage_change;

                        // Storage read map
                        nlohmann::json j_storage_read = nlohmann::json::object();
                        for (const auto& kv : gs_tx.addresses_storage_read) {
                            std::string addr_hex = mvm::address_to_hex_string(kv.first);
                            nlohmann::json j_slots = nlohmann::json::array();
                            for (const auto& slot : kv.second) {
                                j_slots.push_back(mvm::to_hex_string_fixed(slot, 64));
                            }
                            j_storage_read[addr_hex] = j_slots;
                        }
                        j_res["mapStorageRead"] = j_storage_read;

                        // Event logs
                        nlohmann::json j_event_logs = nlohmann::json::array();
                        for (const auto& l : log_handler.logs) {
                            nlohmann::json j_log;
                            j_log["address"] = mvm::address_to_hex_string(l.address);
                            nlohmann::json j_topics = nlohmann::json::array();
                            for (const auto& t : l.topics) {
                                j_topics.push_back(mvm::to_hex_string_fixed(t, 64));
                            }
                            j_log["topics"] = j_topics;
                            j_log["data"] = "0x" + bytes_to_hex(l.data);
                            j_event_logs.push_back(j_log);
                        }
                        j_res["event_logs"] = j_event_logs;

                        // Attach dirty WAL deltas for persistent storage sync on SSD
                        auto dirty_deltas = XapianManager::collectAllDirtyDeltas();
                        if (!dirty_deltas.empty()) {
                            nlohmann::json j_sync = nlohmann::json::array();
                            for (const auto& d : dirty_deltas) {
                                nlohmann::json item;
                                item["contract"] = mvm::address_to_hex_string(d.address);
                                item["dbname"] = d.db_name;
                                item["version"] = d.version;
                                item["type"] = "WAL_APPEND";
                                item["data"] = d.encrypted_hex;
                                j_sync.push_back(item);
                            }
                            j_res["storage_sync"] = j_sync;
                        }

                        std::string out_str = j_res.dump();
                        strncpy(shm_buf, out_str.c_str(), SHM_SIZE - 1);
                    }
                }
                // Legacy command handling for backward compatibility
                else if (input.rfind("DEPLOY:", 0) == 0) {
                    mvm::Address caller = DEFAULT_WALLET;
                    std::string hex_data;
                    
                    size_t pos1 = input.find(':', 7);
                    if (pos1 != std::string::npos && pos1 < 50) {
                        caller = parse_address(input.substr(7, pos1 - 7));
                        hex_data = input.substr(pos1 + 1);
                    } else {
                        hex_data = input.substr(7);
                    }

                    std::vector<uint8_t> bytecode = hex_to_bytes(hex_data);
                    
                    mvm::BlockContext bc_legacy;
                    mvm::MyGlobalState gs_legacy(bc_legacy, true);
                    mvm::VectorLogHandler log_handler;
                    MyLogger native_logger;
                    uint256_t current_tx_hash = intx::uint256(tx_counter++);
                    MyExtension extension_legacy(nullptr, false, &current_tx_hash);
                    mvm::Processor processor_legacy(gs_legacy, log_handler, extension_legacy, native_logger);

                    auto caller_state = gs_legacy.get(caller);
                    uint64_t nonce = static_cast<uint64_t>(caller_state.acc.get_nonce());
                    
                    mvm::Address callee = mvm::generate_address(caller, nonce);
                    caller_state.acc.set_nonce(nonce + 1);
                    
                    mvm::Transaction tx(caller, nonce, 1, 10000000, 0, false);
                    
                    gs_legacy.create(callee, 0, {}, 0);
                    auto callee_state = gs_legacy.get(callee);
                    
                    mvm::ExecResult res = processor_legacy.run(tx, true, caller, callee_state, bytecode, 0, nullptr, false);
                    std::cout << "[EVM-TA] Legacy DEPLOY result: er=" << (int)res.er << " exmsg=" << res.exmsg << std::endl;
                    
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
                        gs_legacy.iterate_storage_changes([&](const mvm::Address &a, const uint256_t &k, const uint256_t &v) {
                            uint8_t b_key[32];
                            mvm::to_big_endian(k, b_key);
                            State::getInstance(a)->insertOrUpdate(State::toKeyType(b_key), v);
                        });

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

                        std::vector<uint8_t> calldata = hex_to_bytes(hex_data);
                        
                        mvm::BlockContext bc_legacy;
                        mvm::MyGlobalState gs_legacy(bc_legacy, true);
                        mvm::VectorLogHandler log_handler;
                        MyLogger native_logger;
                        uint256_t current_tx_hash = intx::uint256(tx_counter++);
                        MyExtension extension_legacy(nullptr, false, &current_tx_hash);
                        mvm::Processor processor_legacy(gs_legacy, log_handler, extension_legacy, native_logger);

                        auto caller_state = gs_legacy.get(caller);
                        uint64_t nonce = static_cast<uint64_t>(caller_state.acc.get_nonce());
                        caller_state.acc.set_nonce(nonce + 1);

                        mvm::Transaction tx(caller, nonce, 1, 10000000, 0, false);
                        auto callee_state = gs_legacy.get(callee);
                        mvm::ExecResult res = processor_legacy.run(tx, false, caller, callee_state, calldata, 0, nullptr, false);
                        
                        if (res.er == mvm::ExitReason::returned) {
                            auto callee_inst = State::getInstance(callee);
                            callee_inst->setBalance(callee_state.acc.get_balance());
                            callee_inst->setNonce(callee_state.acc.get_nonce());

                            auto caller_inst = State::getInstance(caller);
                            caller_inst->setBalance(caller_state.acc.get_balance());
                            caller_inst->setNonce(caller_state.acc.get_nonce());

                            gs_legacy.iterate_storage_changes([&](const mvm::Address &a, const uint256_t &k, const uint256_t &v) {
                                uint8_t b_key[32];
                                mvm::to_big_endian(k, b_key);
                                State::getInstance(a)->insertOrUpdate(State::toKeyType(b_key), v);
                            });

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
                    if (State::instanceExists(target)) {
                        auto inst = State::getInstance(target);
                        std::string out_msg = "WALLET: " + mvm::address_to_hex_string(target) +
                                              " BALANCE: " + mvm::to_hex_string(inst->getBalance()) +
                                              " NONCE: " + std::to_string((unsigned long long)inst->getNonce());
                        strncpy(shm_buf, out_msg.c_str(), SHM_SIZE - 1);
                    } else {
                        strncpy(shm_buf, "WALLET: NOT FOUND", SHM_SIZE - 1);
                    }
                } else {
                    strncpy(shm_buf, "ERROR: Unknown command. Use JSON format or DEPLOY:, CALL:, GET_WALLET:", SHM_SIZE - 1);
                }
            } catch (const mvm::Exception& e) {
                std::string err = "{\"status\":2,\"status_str\":\"THREW\",\"exception\":2,\"exmsg\":\"" + std::string(e.what()) + "\"}";
                strncpy(shm_buf, err.c_str(), SHM_SIZE - 1);
            } catch (const std::exception& e) {
                std::string err = "{\"status\":2,\"status_str\":\"THREW\",\"exception\":-1,\"exmsg\":\"" + std::string(e.what()) + "\"}";
                strncpy(shm_buf, err.c_str(), SHM_SIZE - 1);
            } catch (...) {
                std::string err = "{\"status\":2,\"status_str\":\"THREW\",\"exception\":-1,\"exmsg\":\"Fatal Unknown Exception\"}";
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
