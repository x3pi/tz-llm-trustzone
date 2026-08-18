#include "mvm_linker.hpp"
#include <iostream>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cstdint>

extern "C" {

struct GlobalStateGet_return {
  int status;
  unsigned char *balance_p;
  unsigned char *nonce;
  unsigned char *code_p;
  int code_length;
};

struct GlobalStateGet_return GlobalStateGet(unsigned char *mvmId, unsigned char *) {
    struct GlobalStateGet_return ret;
    ret.status = 0; // 0 means not found, which causes my_global_state.cpp to create a new Account.
    ret.balance_p = nullptr;
    ret.nonce = nullptr;
    ret.code_p = nullptr;
    ret.code_length = 0;
    return ret;
}

#ifdef MVM_LINKER_BUILD
struct GetStorageValue_return GetStorageValue(unsigned char *mvmId, unsigned char *, unsigned char *) {
    struct GetStorageValue_return ret;
    ret.status = STORAGE_NOT_FOUND;
    ret.value = nullptr;
    return ret;
}
#endif

void ClearProcessingPointers(unsigned char *mvmId) {}

struct Extension_return ExtensionCallGetApi(unsigned char *, int) {
    struct Extension_return ret;
    ret.data_p = nullptr;
    ret.data_size = 0;
    return ret;
}

struct Extension_return ExtensionExtractJsonField(unsigned char *, int) {
    struct Extension_return ret;
    ret.data_p = nullptr;
    ret.data_size = 0;
    return ret;
}

struct Extension_return ExtensionBlst(unsigned char *, int) {
    struct Extension_return ret;
    ret.data_p = nullptr;
    ret.data_size = 0;
    return ret;
}

struct Extension_return ExtensionGetOrCreateSimpleDb(unsigned char *, int, unsigned char *, unsigned char *) {
    struct Extension_return ret;
    ret.data_p = nullptr;
    ret.data_size = 0;
    return ret;
}

struct Value_return GetBlockHash(int) {
    struct Value_return ret;
    ret.success = true;
    ret.data_p = (unsigned char*)malloc(32);
    memset(ret.data_p, 0, 32);
    ret.data_size = 32;
    return ret;
}

struct Value_return GetChainId() {
    struct Value_return ret;
    ret.success = true;
    ret.data_p = (unsigned char*)malloc(32);
    memset(ret.data_p, 0, 32);
    ret.data_p[31] = 1; // Chain ID 1
    ret.data_size = 32;
    return ret;
}

struct Value_return GetCrossChainSender(unsigned char *mvmId) {
    struct Value_return ret;
    ret.success = false;
    ret.data_p = nullptr;
    ret.data_size = 0;
    return ret;
}

struct Value_return GetCrossChainSourceId(unsigned char *mvmId) {
    struct Value_return ret;
    ret.success = false;
    ret.data_p = nullptr;
    ret.data_size = 0;
    return ret;
}



void GoLogString(int, char *msg) {
    std::cout << "[EVM-TA GoLog] " << msg << std::endl;
}

void GoLogBytes(int, unsigned char *, int) {}

struct ExecuteResult *testMemLeak() { return nullptr; }
void testMemLeakGS(int total_address, unsigned char *b_addresses) {}
void clearAllStateInstances() {}
void updateStateNonce(unsigned char *b_address, unsigned long long nonce) {}
void updateStateBalance(unsigned char *b_address, unsigned char *b_balance) {}

void MVM_cancelTransaction(unsigned char *mvmId) {}
void MVM_commitAllXapian() {}

struct ExportedXapianLogArray MVM_exportAllXapianLogs() {
    struct ExportedXapianLogArray ret;
    ret.data = nullptr;
    ret.count = 0;
    return ret;
}

void MVM_freeExportedXapianLogs(struct ExportedXapianLogArray array) {}

int commit_full_db(unsigned char *mvmId, unsigned char *txHashes, int numHashes) { return 1; }
int revert_full_db(unsigned char *mvmId) { return 1; }
void clear_xapian_tx_buffer(unsigned char *b_tx_hash) {}
void commit_xapian_tx_buffer(unsigned char *b_tx_hash) {}
int ReplayFullDbLogs(LogReplayEntryC *entries, int num_entries) { return 1; }

} // extern "C"



// UUID mocks for Xapian
extern "C" {
    typedef unsigned char uuid_t[16];
    
    void uuid_generate_random(uuid_t out) {
        for (int i = 0; i < 16; ++i) {
            out[i] = rand() % 256;
        }
        out[6] = (out[6] & 0x0f) | 0x40;
        out[8] = (out[8] & 0x3f) | 0x80;
    }
    
    void uuid_unparse_lower(const uuid_t uu, char *out) {
        sprintf(out, "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
            uu[0], uu[1], uu[2], uu[3],
            uu[4], uu[5],
            uu[6], uu[7],
            uu[8], uu[9],
            uu[10], uu[11], uu[12], uu[13], uu[14], uu[15]);
    }
}
