#pragma once

#include <vector>
#include <string>

// ---------------------------------------------------------------------------
// AES-128 Encryption/Decryption - WYD Compatible Implementation
// Used for packet encryption and secure communication
// NOTE: This is a simplified implementation for WYD compatibility
// For production, consider using a validated crypto library
// ---------------------------------------------------------------------------

class AES
{
public:
    // Key size constants
    static constexpr int KEY_SIZE_128 = 16;
    static constexpr int KEY_SIZE_192 = 24;
    static constexpr int KEY_SIZE_256 = 32;
    static constexpr int BLOCK_SIZE = 16;

    // Initialize AES with given key
    static void SetKey(const unsigned char* key, int keySize = KEY_SIZE_128);
    
    // Encrypt single block (16 bytes)
    static void EncryptBlock(const unsigned char* input, unsigned char* output);
    
    // Decrypt single block (16 bytes)
    static void DecryptBlock(const unsigned char* input, unsigned char* output);
    
    // Encrypt data (handles padding)
    static void Encrypt(const unsigned char* input, unsigned char* output, 
                       size_t length);
    
    // Decrypt data (handles padding removal)
    static void Decrypt(const unsigned char* input, unsigned char* output, 
                       size_t length);
    
    // Encrypt string to hex string
    static std::string EncryptString(const std::string& plaintext);
    
    // Decrypt hex string to string
    static std::string DecryptString(const std::string& ciphertext);

private:
    static unsigned char s_RoundKey[176]; // For 128-bit key
    static int s_KeySize;
    static bool s_KeyInitialized;
    
    // Internal AES operations
    static void KeyExpansion(const unsigned char* key);
    static void SubBytes(unsigned char* state);
    static void ShiftRows(unsigned char* state);
    static void MixColumns(unsigned char* state);
    static void AddRoundKey(unsigned char* state, int round);
    
    static void InvSubBytes(unsigned char* state);
    static void InvShiftRows(unsigned char* state);
    static void InvMixColumns(unsigned char* state);
    
    static unsigned char sBox[256];
    static unsigned char rsBox[256];
    static unsigned char Rcon[11];
};
