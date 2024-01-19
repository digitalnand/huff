#pragma once

#include <cstdint>
#include <fstream>
#include <unordered_map>

#include "types.hpp"

struct Encoder {
    private:
        std::ifstream target;
        std::string target_path;

        std::unordered_map<char, uint32_t> extract_frequencies();
        Node build_huffman_tree(const std::unordered_map<char, uint32_t>&);
        std::vector<std::pair<char, uint32_t>> get_huffman_codes_length(const Node&);
        std::unordered_map<char, std::string> generate_canonical_codes(const std::vector<std::pair<char, uint32_t>>&);
        std::vector<char> encode_codes_length(const std::unordered_map<char, std::string>&);
        std::vector<char> encode_content(const std::unordered_map<char, std::string>&);

    public:
        Encoder(const std::string&);
        void create_compressed_file();
};
