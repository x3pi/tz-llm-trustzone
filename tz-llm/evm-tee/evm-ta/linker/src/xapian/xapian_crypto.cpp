#include "xapian_crypto.h"
#include <cstring>
#include <sstream>
#include <iomanip>

namespace XapianCrypto {

static const std::array<uint8_t, KEY_SIZE> TEE_MASTER_STORAGE_KEY = {
    0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
    0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c,
    0x76, 0x2e, 0x71, 0x60, 0xf3, 0x8b, 0x4d, 0xa5,
    0x6a, 0x78, 0x4d, 0x90, 0x45, 0x19, 0x0c, 0xfe
};

const std::array<uint8_t, KEY_SIZE>& getTeeStorageKey() {
    return TEE_MASTER_STORAGE_KEY;
}

// -----------------------------------------------------------------------------
// SHA256 Implementation (Self-contained for TEE)
// -----------------------------------------------------------------------------
namespace SHA256 {
    static inline uint32_t rotr(uint32_t x, uint32_t n) {
        return (x >> n) | (x << (32 - n));
    }
    static inline uint32_t ch(uint32_t x, uint32_t y, uint32_t z) {
        return (x & y) ^ (~x & z);
    }
    static inline uint32_t maj(uint32_t x, uint32_t y, uint32_t z) {
        return (x & y) ^ (x & z) ^ (y & z);
    }
    static inline uint32_t sig0(uint32_t x) {
        return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22);
    }
    static inline uint32_t sig1(uint32_t x) {
        return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25);
    }
    static inline uint32_t gam0(uint32_t x) {
        return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3);
    }
    static inline uint32_t gam1(uint32_t x) {
        return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10);
    }

    static const uint32_t K[64] = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
        0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
        0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
        0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
        0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
        0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
        0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
        0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
        0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
    };

    struct Context {
        uint32_t state[8];
        uint64_t count;
        uint8_t buffer[64];
    };

    static void init(Context& ctx) {
        ctx.state[0] = 0x6a09e667;
        ctx.state[1] = 0xbb67ae85;
        ctx.state[2] = 0x3c6ef372;
        ctx.state[3] = 0xa54ff53a;
        ctx.state[4] = 0x510e527f;
        ctx.state[5] = 0x9b05688c;
        ctx.state[6] = 0x1f83d9ab;
        ctx.state[7] = 0x5be0cd19;
        ctx.count = 0;
    }

    static void transform(Context& ctx, const uint8_t* data) {
        uint32_t a = ctx.state[0], b = ctx.state[1], c = ctx.state[2], d = ctx.state[3];
        uint32_t e = ctx.state[4], f = ctx.state[5], g = ctx.state[6], h = ctx.state[7];
        uint32_t w[64];

        for (int i = 0; i < 16; ++i) {
            w[i] = ((uint32_t)data[i * 4] << 24) |
                   ((uint32_t)data[i * 4 + 1] << 16) |
                   ((uint32_t)data[i * 4 + 2] << 8) |
                   ((uint32_t)data[i * 4 + 3]);
        }
        for (int i = 16; i < 64; ++i) {
            w[i] = gam1(w[i - 2]) + w[i - 7] + gam0(w[i - 15]) + w[i - 16];
        }

        for (int i = 0; i < 64; ++i) {
            uint32_t t1 = h + sig1(e) + ch(e, f, g) + K[i] + w[i];
            uint32_t t2 = sig0(a) + maj(a, b, c);
            h = g;
            g = f;
            f = e;
            e = d + t1;
            d = c;
            c = b;
            b = a;
            a = t1 + t2;
        }

        ctx.state[0] += a;
        ctx.state[1] += b;
        ctx.state[2] += c;
        ctx.state[3] += d;
        ctx.state[4] += e;
        ctx.state[5] += f;
        ctx.state[6] += g;
        ctx.state[7] += h;
    }

    static void update(Context& ctx, const uint8_t* data, size_t len) {
        size_t buffer_idx = (size_t)(ctx.count & 63);
        ctx.count += len;
        size_t part_len = 64 - buffer_idx;

        size_t i = 0;
        if (buffer_idx > 0 && len >= part_len) {
            std::memcpy(&ctx.buffer[buffer_idx], data, part_len);
            transform(ctx, ctx.buffer);
            i = part_len;
            buffer_idx = 0;
        }

        for (; i + 63 < len; i += 64) {
            transform(ctx, &data[i]);
        }

        if (i < len) {
            std::memcpy(&ctx.buffer[buffer_idx], &data[i], len - i);
        }
    }

    static void final(Context& ctx, uint8_t* digest) {
        uint8_t final_count[8];
        uint64_t bits = ctx.count * 8;
        for (int i = 0; i < 8; ++i) {
            final_count[7 - i] = (uint8_t)(bits >> (i * 8));
        }

        size_t buffer_idx = (size_t)(ctx.count & 63);
        uint8_t pad[64] = {0x80};
        size_t pad_len = (buffer_idx < 56) ? (56 - buffer_idx) : (120 - buffer_idx);
        update(ctx, pad, pad_len);
        update(ctx, final_count, 8);

        for (int i = 0; i < 8; ++i) {
            digest[i * 4]     = (uint8_t)(ctx.state[i] >> 24);
            digest[i * 4 + 1] = (uint8_t)(ctx.state[i] >> 16);
            digest[i * 4 + 2] = (uint8_t)(ctx.state[i] >> 8);
            digest[i * 4 + 3] = (uint8_t)(ctx.state[i]);
        }
    }

    static std::array<uint8_t, 32> hash(const uint8_t* data, size_t len) {
        Context ctx;
        init(ctx);
        update(ctx, data, len);
        std::array<uint8_t, 32> out;
        final(ctx, out.data());
        return out;
    }
}

// -----------------------------------------------------------------------------
// HMAC-SHA256 Implementation
// -----------------------------------------------------------------------------
static std::array<uint8_t, 32> hmac_sha256(const uint8_t* key, size_t key_len, const uint8_t* data, size_t data_len) {
    uint8_t k_pad[64] = {0};
    if (key_len > 64) {
        auto key_hash = SHA256::hash(key, key_len);
        std::memcpy(k_pad, key_hash.data(), 32);
    } else {
        std::memcpy(k_pad, key, key_len);
    }

    uint8_t i_pad[64];
    uint8_t o_pad[64];
    for (int i = 0; i < 64; ++i) {
        i_pad[i] = k_pad[i] ^ 0x36;
        o_pad[i] = k_pad[i] ^ 0x5c;
    }

    SHA256::Context ctx_inner;
    SHA256::init(ctx_inner);
    SHA256::update(ctx_inner, i_pad, 64);
    SHA256::update(ctx_inner, data, data_len);
    uint8_t inner_hash[32];
    SHA256::final(ctx_inner, inner_hash);

    SHA256::Context ctx_outer;
    SHA256::init(ctx_outer);
    SHA256::update(ctx_outer, o_pad, 64);
    SHA256::update(ctx_outer, inner_hash, 32);
    std::array<uint8_t, 32> out;
    SHA256::final(ctx_outer, out.data());
    return out;
}

// -----------------------------------------------------------------------------
// ChaCha20 Implementation (RFC 7539)
// -----------------------------------------------------------------------------
namespace ChaCha20 {
    static inline uint32_t rotl32(uint32_t x, uint32_t n) {
        return (x << n) | (x >> (32 - n));
    }

    static inline void quarter_round(uint32_t& a, uint32_t& b, uint32_t& c, uint32_t& d) {
        a += b; d ^= a; d = rotl32(d, 16);
        c += d; b ^= c; b = rotl32(b, 12);
        a += b; d ^= a; d = rotl32(d, 8);
        c += d; b ^= c; b = rotl32(b, 7);
    }

    static void chacha20_block(uint32_t out[16], const uint32_t in[16]) {
        for (int i = 0; i < 16; ++i) out[i] = in[i];
        for (int i = 0; i < 10; ++i) {
            // Column round
            quarter_round(out[0], out[4], out[8],  out[12]);
            quarter_round(out[1], out[5], out[9],  out[13]);
            quarter_round(out[2], out[6], out[10], out[14]);
            quarter_round(out[3], out[7], out[11], out[15]);
            // Diagonal round
            quarter_round(out[0], out[5], out[10], out[15]);
            quarter_round(out[1], out[6], out[11], out[12]);
            quarter_round(out[2], out[7], out[8],  out[13]);
            quarter_round(out[3], out[4], out[9],  out[14]);
        }
        for (int i = 0; i < 16; ++i) out[i] += in[i];
    }

    static void process(const uint8_t* key, const uint8_t* nonce, uint32_t initial_counter,
                        const uint8_t* in, uint8_t* out, size_t len) {
        uint32_t state[16];
        // "expand 32-byte k" constants
        state[0] = 0x61707865;
        state[1] = 0x3320646e;
        state[2] = 0x79622d32;
        state[3] = 0x6b206574;

        for (int i = 0; i < 8; ++i) {
            state[4 + i] = ((uint32_t)key[i * 4]) |
                           ((uint32_t)key[i * 4 + 1] << 8) |
                           ((uint32_t)key[i * 4 + 2] << 16) |
                           ((uint32_t)key[i * 4 + 3] << 24);
        }

        state[12] = initial_counter;
        for (int i = 0; i < 3; ++i) {
            state[13 + i] = ((uint32_t)nonce[i * 4]) |
                            ((uint32_t)nonce[i * 4 + 1] << 8) |
                            ((uint32_t)nonce[i * 4 + 2] << 16) |
                            ((uint32_t)nonce[i * 4 + 3] << 24);
        }

        size_t offset = 0;
        uint32_t block[16];
        uint8_t key_stream[64];

        while (offset < len) {
            chacha20_block(block, state);
            state[12]++; // Increment counter

            for (int i = 0; i < 16; ++i) {
                key_stream[i * 4]     = (uint8_t)(block[i]);
                key_stream[i * 4 + 1] = (uint8_t)(block[i] >> 8);
                key_stream[i * 4 + 2] = (uint8_t)(block[i] >> 16);
                key_stream[i * 4 + 3] = (uint8_t)(block[i] >> 24);
            }

            size_t chunk = std::min((size_t)64, len - offset);
            for (size_t i = 0; i < chunk; ++i) {
                out[offset + i] = in[offset + i] ^ key_stream[i];
            }
            offset += chunk;
        }
    }
}

// -----------------------------------------------------------------------------
// Envelope Structure (Header + Payload)
// -----------------------------------------------------------------------------
// Layout:
// [0..3]:   Magic (0x545A5850 - "TZXP")
// [4..7]:   Version (1)
// [8..15]:  Sequence / Storage Version (uint64_t little-endian)
// [16..27]: Nonce (12 bytes)
// [28..59]: HMAC-SHA256 Tag (32 bytes over Header + Ciphertext)
// [60..]:   ChaCha20 Ciphertext

std::vector<uint8_t> encrypt(const std::vector<uint8_t>& plaintext, uint64_t seq_num) {
    const auto& key = getTeeStorageKey();

    // Deterministic Nonce based on seq_num and key derivation
    std::vector<uint8_t> nonce(NONCE_SIZE, 0);
    for (int i = 0; i < 8; ++i) {
        nonce[i] = (uint8_t)(seq_num >> (i * 8));
    }
    nonce[8] = 0xAA; nonce[9] = 0x55; nonce[10] = 0x12; nonce[11] = 0x34;

    std::vector<uint8_t> ciphertext(plaintext.size());
    ChaCha20::process(key.data(), nonce.data(), 1, plaintext.data(), ciphertext.data(), plaintext.size());

    // Build envelope header (60 bytes)
    std::vector<uint8_t> envelope;
    envelope.reserve(60 + ciphertext.size());

    // Magic & Version
    envelope.push_back((uint8_t)(MAGIC_HEADER));
    envelope.push_back((uint8_t)(MAGIC_HEADER >> 8));
    envelope.push_back((uint8_t)(MAGIC_HEADER >> 16));
    envelope.push_back((uint8_t)(MAGIC_HEADER >> 24));

    envelope.push_back((uint8_t)(FORMAT_VERSION));
    envelope.push_back((uint8_t)(FORMAT_VERSION >> 8));
    envelope.push_back((uint8_t)(FORMAT_VERSION >> 16));
    envelope.push_back((uint8_t)(FORMAT_VERSION >> 24));

    // Sequence / Version
    for (int i = 0; i < 8; ++i) {
        envelope.push_back((uint8_t)(seq_num >> (i * 8)));
    }

    // Nonce (12B)
    envelope.insert(envelope.end(), nonce.begin(), nonce.end());

    // Placeholder for Tag (32B)
    size_t tag_offset = envelope.size();
    envelope.insert(envelope.end(), TAG_SIZE, 0);

    // Append Ciphertext
    envelope.insert(envelope.end(), ciphertext.begin(), ciphertext.end());

    // Compute HMAC over Header(excluding tag) + Ciphertext
    std::vector<uint8_t> authenticated_data;
    authenticated_data.insert(authenticated_data.end(), envelope.begin(), envelope.begin() + 28);
    authenticated_data.insert(authenticated_data.end(), ciphertext.begin(), ciphertext.end());

    auto tag = hmac_sha256(key.data(), key.size(), authenticated_data.data(), authenticated_data.size());
    std::memcpy(&envelope[tag_offset], tag.data(), TAG_SIZE);

    return envelope;
}

std::optional<std::vector<uint8_t>> decrypt(const std::vector<uint8_t>& envelope_bytes, uint64_t expected_min_seq) {
    if (envelope_bytes.size() < 60) {
        return std::nullopt; // Too short to contain valid header
    }

    const auto& key = getTeeStorageKey();

    // Verify Magic
    uint32_t magic = (uint32_t)envelope_bytes[0] |
                    ((uint32_t)envelope_bytes[1] << 8) |
                    ((uint32_t)envelope_bytes[2] << 16) |
                    ((uint32_t)envelope_bytes[3] << 24);
    if (magic != MAGIC_HEADER) {
        return std::nullopt;
    }

    // Verify Version
    uint32_t version = (uint32_t)envelope_bytes[4] |
                      ((uint32_t)envelope_bytes[5] << 8) |
                      ((uint32_t)envelope_bytes[6] << 16) |
                      ((uint32_t)envelope_bytes[7] << 24);
    if (version != FORMAT_VERSION) {
        return std::nullopt;
    }

    // Extract Sequence number
    uint64_t seq = 0;
    for (int i = 0; i < 8; ++i) {
        seq |= ((uint64_t)envelope_bytes[8 + i]) << (i * 8);
    }
    if (seq < expected_min_seq) {
        return std::nullopt; // Anti-rollback violation
    }

    // Extract Nonce (12 bytes)
    const uint8_t* nonce = &envelope_bytes[16];

    // Extract Tag (32 bytes)
    const uint8_t* tag_in_envelope = &envelope_bytes[28];

    // Extract Ciphertext
    const uint8_t* ciphertext = &envelope_bytes[60];
    size_t ciphertext_len = envelope_bytes.size() - 60;

    // Verify HMAC Tag
    std::vector<uint8_t> authenticated_data;
    authenticated_data.insert(authenticated_data.end(), envelope_bytes.begin(), envelope_bytes.begin() + 28);
    authenticated_data.insert(authenticated_data.end(), ciphertext, ciphertext + ciphertext_len);

    auto computed_tag = hmac_sha256(key.data(), key.size(), authenticated_data.data(), authenticated_data.size());

    // Constant-time compare
    uint8_t diff = 0;
    for (size_t i = 0; i < TAG_SIZE; ++i) {
        diff |= (tag_in_envelope[i] ^ computed_tag[i]);
    }
    if (diff != 0) {
        return std::nullopt; // Tag verification failed (tampered data)
    }

    // Decrypt Ciphertext with ChaCha20
    std::vector<uint8_t> plaintext(ciphertext_len);
    ChaCha20::process(key.data(), nonce, 1, ciphertext, plaintext.data(), ciphertext_len);

    return plaintext;
}

std::string bytesToHex(const std::vector<uint8_t>& bytes) {
    static const char hex_chars[] = "0123456789abcdef";
    std::string hex;
    hex.reserve(bytes.size() * 2);
    for (uint8_t b : bytes) {
        hex.push_back(hex_chars[(b >> 4) & 0x0f]);
        hex.push_back(hex_chars[b & 0x0f]);
    }
    return hex;
}

std::vector<uint8_t> hexToBytes(const std::string& hex) {
    std::vector<uint8_t> bytes;
    size_t start = 0;
    if (hex.size() >= 2 && hex[0] == '0' && (hex[1] == 'x' || hex[1] == 'X')) {
        start = 2;
    }
    bytes.reserve((hex.size() - start) / 2);
    for (size_t i = start; i + 1 < hex.size(); i += 2) {
        uint8_t high = 0, low = 0;
        char c1 = hex[i], c2 = hex[i + 1];
        if (c1 >= '0' && c1 <= '9') high = c1 - '0';
        else if (c1 >= 'a' && c1 <= 'f') high = c1 - 'a' + 10;
        else if (c1 >= 'A' && c1 <= 'F') high = c1 - 'A' + 10;

        if (c2 >= '0' && c2 <= '9') low = c2 - '0';
        else if (c2 >= 'a' && c2 <= 'f') low = c2 - 'a' + 10;
        else if (c2 >= 'A' && c2 <= 'F') low = c2 - 'A' + 10;

        bytes.push_back((high << 4) | low);
    }
    return bytes;
}

} // namespace XapianCrypto
