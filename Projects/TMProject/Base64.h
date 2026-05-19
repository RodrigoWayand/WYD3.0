#pragma once

#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Base64 Encoding/Decoding - WYD Compatible Implementation
// Used for packet encoding and binary data serialization
// ---------------------------------------------------------------------------

class Base64
{
public:
    // Encode binary data to Base64 string
    static std::string Encode(const unsigned char* data, size_t len);
    
    // Decode Base64 string to binary data
    static std::vector<unsigned char> Decode(const std::string& encoded);
    
    // Get encoded length for given input length
    static size_t GetEncodedLength(size_t inputLen);
    
    // Get decoded length for given input length
    static size_t GetDecodedLength(const std::string& encoded);

private:
    static const char* GetEncodingTable();
    static const char* GetDecodingTable();
};
