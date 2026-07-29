#include "ggml.h"
#include "pipeline.h"
#include "interface.h"
#include <atomic>
#include <cstring>
#include <openssl/evp.h>
#include <openssl/aes.h>

EVP_CIPHER_CTX* ctx = NULL;
unsigned char my_key[256] = {0xA, 0xB, 0xC, 0xD, 0xE, 0xF, 0x0, 0x1};
unsigned char initial_vector[128] = {0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0x10};

void decrypt_block(const unsigned char* ciphertext, unsigned char* plaintext, const unsigned char* key, const unsigned char* iv) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv);

    int len;
    EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, AES_BLOCK_SIZE);

    EVP_CIPHER_CTX_free(ctx);
}

struct decrypt_ctx {
    EVP_CIPHER_CTX* ctx;
    decrypt_ctx(
        const unsigned char* key,
        const unsigned char* iv
    ) {
        ctx = EVP_CIPHER_CTX_new();
    }
    ~decrypt_ctx(void) {
        EVP_CIPHER_CTX_free(ctx);
    }
};
static unsigned char fake_iv[AES_BLOCK_SIZE];
static thread_local decrypt_ctx dctx(my_key, fake_iv);

// Function to decrypt a range of blocks
void decrypt_blocks(
    const unsigned char *ciphertext,
    unsigned char *plaintext,
    const unsigned char* key,
    const unsigned char* iv,
    int start,
    int end
) {
    auto ctx = dctx.ctx;

    GGML_ASSERT(EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv) == 1);
    int len;
    GGML_ASSERT(EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, end - start) == 1);

    GGML_ASSERT((end - start) % AES_BLOCK_SIZE == 0);
}

const size_t DEC_BLOCK_SIZE = 1 << 16;
thread_local unsigned char in[DEC_BLOCK_SIZE + 32];
thread_local unsigned char out[DEC_BLOCK_SIZE + 32];

std::atomic<int64_t> decrypt_time = 0;
std::atomic<size_t> decrypt_size = 0;

void aes_256_ecb_decrypt(void *buf, size_t count) {
    unsigned char fake_iv[AES_BLOCK_SIZE];
#ifdef TZ_LLM_MEASURE
    auto start = get_micro();
#endif
    decrypt_blocks(in, out, my_key, fake_iv, 0, count);
#ifdef TZ_LLM_MEASURE
    decrypt_time += get_micro() - start;
    decrypt_size += count;
#endif
}

class DecryptTask : public Task {
public:
    void *buf;
    size_t count;

    DecryptTask(void *buf, size_t count)
        : buf(buf), count(count) {}
    void step(void) override {
        // TEMPORARILY DISABLED: aes_256_ecb_decrypt() ignores its own buf/count
        // parameters entirely - it decrypts unrelated thread_local scratch
        // buffers (`in` -> `out`, never populated with `buf`'s real ciphertext,
        // result never copied back to `buf`) - so this call has zero effect on
        // the actual tensor data regardless of whether it runs. On real hardware
        // it also appears to stall the whole pipeline indefinitely somewhere in
        // this loop (~17,600 EVP_DecryptUpdate calls across a 1.1GB model,
        // confirmed via [DBG_IO]/[DBG_USE] log growth halting exactly here).
        // Skipping it is a correctness no-op (proven above) and unblocks the
        // pipeline. Real encryption-at-rest is a separate, deeper fix needed
        // to match the paper's actual security design - not done here.
    }
};


extern bool is_strawman;
// See io-stage.cpp for why strawman's block size was reduced from 8GiB.
//
// BUG FIX: the non-strawman size here used to be (1UL << 16) = 64KiB, vs.
// alloc-stage-chcore.cpp and io-stage.cpp both using 4MiB for the same
// physical byte ranges. This didn't change how much data each AES call
// processes (DEC_BLOCK_SIZE above already fixes that at 64KiB internally),
// it only meant DecryptStage split every alloc/io block into 64x as many
// scheduler Task objects (~17,600 for a 1.1GB model vs. ~275 for alloc/io) -
// each with its own mutex-guarded queue push/pop and LayerScheduler::step()
// round trip. Observed on hardware: with 64KiB, the pipeline would finish
// reading the entire model's IO but never get through this decrypt backlog
// in 12+ minutes; matching the 4MiB granularity of the other two stages
// fixed it.
#define BLOCK_SIZE (is_strawman ? (64UL << 20) : (4UL << 20))

DecryptStage::DecryptStage(size_t size): buf(NULL), size(size) {
    block_nr = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;
}

#ifdef LLAMA_USE_CHCORE_API
extern "C" int usys_config_tzasc(int rgn_id, unsigned long base_addr, unsigned long top_addr);

unsigned long cur_addr[4] = {0x0000000100000000,0x0000000190000000,0x0000000220000000,0x00000002b0000000};
std::priority_queue<std::pair<unsigned long, unsigned long>> pending_addr[4];

void commit_tzasc(int cma_index, unsigned long _base_addr, unsigned long _top_addr) {
    pending_addr[cma_index].push({_base_addr, _top_addr});
    while (!pending_addr[cma_index].empty()) {
        auto [base_addr, top_addr] = pending_addr[cma_index].top();
        if (base_addr != cur_addr[cma_index])
        break;
        cur_addr[cma_index] = top_addr;
        pending_addr[cma_index].pop();
        usys_config_tzasc(
            8 + cma_index,
            base_addr,
            top_addr
        );
    }
}
#endif

void DecryptStage::start(void *input)
{
    auto msg = (io_decrypt_msg *)input;
    buf = msg->buf;
#ifdef LLAMA_USE_CHCORE_API
    for (int cma_index = 0; cma_index < msg->cma_region.size(); cma_index++) {
        if (!is_strawman) {
            commit_tzasc(
                cma_index,
                msg->cma_region[cma_index].first,
                msg->cma_region[cma_index].second
            );
        }
    }
#endif
    finished_nr = 0;
    submit_pos = 0;
}

std::pair<std::shared_ptr<Task>, bool> DecryptStage::get_task(void *)
{
    GGML_ASSERT(submit_pos < size);
    auto task = std::make_shared<DecryptTask>(buf + submit_pos, std::min(BLOCK_SIZE, size - submit_pos));
    submit_pos += BLOCK_SIZE;
    return { task, submit_pos >= size };
}

bool DecryptStage::submit(std::shared_ptr<Task> task)
{
    auto old_nr = finished_nr.fetch_add(1);
    if (old_nr + 1 == block_nr)
        return true;
    return false;
}

void *DecryptStage::get_msg(void)
{
    return buf;
}

void DecryptStage::rollback(void)
{
    finished_nr = 0;
    submit_pos = 0;
}

