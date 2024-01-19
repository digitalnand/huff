#pragma once

#include <cstdint>
#include <fstream>
#include <unordered_map>

#include "types.hpp"

std::unordered_map<char, uint32_t> extract_frequencies(std::ifstream&);

Node build_huffman_tree(const std::unordered_map<char, uint32_t>&);

std::vector<std::pair<char, uint32_t>> get_huffman_codes_length(const Node&);

std::unordered_map<char, std::string> generate_canonical_codes(const std::vector<std::pair<char, uint32_t>>&);

std::vector<char> encode_codes_length(const std::unordered_map<char, std::string>&);

std::vector<char> encode_content(std::ifstream&, const std::unordered_map<char, std::string>&);

void create_compressed_file(const std::string&);
