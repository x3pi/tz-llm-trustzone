#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <array>
#include <optional>

namespace XapianCrypto {

constexpr uint32_t MAGIC_HEADER = 0x545A5850; // "TZXP" (TrustZone XaPian)
constexpr uint32_t FORMAT_VERSION = 1;
constexpr size_t KEY_SIZE = 32;
constexpr size_t NONCE_SIZE = 12;
constexpr size_t TAG_SIZE = 32; // SHA256 HMAC tag

// Retrieve static TEE master key (protected in Secure World RAM)
const std::array<uint8_t, KEY_SIZE>& getTeeStorageKey();

// Encrypt plaintext with ChaCha20 + HMAC-SHA256 authenticated envelope
std::vector<uint8_t> encrypt(const std::vector<uint8_t>& plaintext, uint64_t seq_num);

// Decrypt authenticated envelope. Returns std::nullopt if integrity check fails
std::optional<std::vector<uint8_t>> decrypt(const std::vector<uint8_t>& envelope_bytes, uint64_t expected_min_seq = 0);

// Hex conversion utilities
std::string bytesToHex(const std::vector<uint8_t>& bytes);
std::vector<uint8_t> hexToBytes(const std::string& hex);

} // namespace XapianCrypto
