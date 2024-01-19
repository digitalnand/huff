#pragma once

#include <cstdint>
#include <bitset>
#include <unordered_map>
#include <fstream>
#include <vector>

#include "types.hpp"

struct Decoder {
    private:
        std::ifstream target;
        std::string target_path;
        size_t index = 0;

        std::bitset<8> next_byte();
        uint32_t decode_bits_length();
        std::vector<std::pair<char, uint32_t>> decode_codes_length(const uint32_t&);
        std::unordered_map<char, std::string> regenerate_codes(const std::vector<std::pair<char, uint32_t>>&);
        Node recreate_huffman_tree(const std::unordered_map<char, std::string>&);
        std::string decode_content(const Node&);

    public:
        Decoder(const std::string&);
        void create_decompressed_file();
};
