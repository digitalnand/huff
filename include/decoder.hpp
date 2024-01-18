#pragma once

#include <cstdint>
#include <bitset>
#include <unordered_map>
#include <fstream>
#include <vector>

#include "types.hpp"

struct Decoder {
    private: std::ifstream file;
    private: std::string target_file_path;

    private: size_t bits_length = 0;
    private: size_t index = 0;

    public: Decoder(const std::string&);
    private: std::bitset<8> next_byte();
    private: void decode_bits_length();
    private: std::vector<std::pair<char, uint16_t>> decode_codes_length();
    private: std::unordered_map<char, std::string> generate_codes();
    private: Node recreate_huffman_tree();
    private: std::string decode_content();
    public: void create_decompressed_file();
};
