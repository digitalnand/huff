#include <algorithm>
#include <cmath>

#include "decoder.hpp"

Decoder::Decoder(const std::string& compressed_file_path) {
    file.open(compressed_file_path, std::ios::binary);
    target_file_path = compressed_file_path;
}

std::bitset<8> Decoder::next_byte() {
    file.clear();
    file.seekg(index, std::ios::beg);

    char buffer[8];
    file.read(buffer, 8);
    index++;

    return std::bitset<8>(*buffer);
}

uint32_t Decoder::decode_bits_length() {
    return next_byte().to_ulong();
}

std::vector<std::pair<char, uint32_t>> Decoder::decode_codes_length(const uint32_t& bits_length) {
    std::vector<std::pair<char, uint32_t>> codes_length;

    std::string carry;
    char current = FIRST_CHARACTER;

    while(index <= std::ceil(SUPPORTED_CHARACTERS / 8.0) * bits_length) {
        auto byte = next_byte().to_string();

        if(!carry.empty()) {
            byte = carry + byte;
            carry.clear();
        }

        for(size_t position = 0; position < byte.size(); position += bits_length) {
            const auto binary_length = byte.substr(position, bits_length);

            if(binary_length.size() < bits_length) {
                carry = binary_length;
                break;
            }

            const uint32_t code_length = std::bitset<8>(binary_length).to_ulong();
            if(code_length > 0) codes_length.emplace_back(current, code_length);
            current++;
        }
    }

    std::sort(codes_length.begin(), codes_length.end(), [](const auto& left, const auto& right) {
        const auto& [left_symbol, left_length] = left;
        const auto& [right_symbol, right_length] = right;
        return (left_length == right_length) ? (left_symbol < right_symbol) : (left_length < right_length);
    });

    return codes_length;
}

std::unordered_map<char, std::string> Decoder::regenerate_codes(const std::vector<std::pair<char, uint32_t>>& codes_length) {
    std::unordered_map<char, std::string> canonical_codes;

    const auto& [front_symbol, front_length] = codes_length.front();

    for(size_t index = 0 ; index < front_length; index++)
        canonical_codes[front_symbol] += '0';

    uint32_t last_code = 0;
    auto last_length = front_length;

    for(size_t index = 1; index < codes_length.size(); index++) {
        const auto& [next_symbol, next_length] = codes_length.at(index);

        last_code = (last_code + 1) << (next_length - last_length);;
        last_length = next_length;

        canonical_codes[next_symbol] = std::bitset<MAX_BITS>(last_code).to_string().substr(MAX_BITS - next_length);
    }

    return canonical_codes;
}

Node Decoder::recreate_huffman_tree(const std::unordered_map<char, std::string>& code_table) {
    Node root{};

    for(const auto& [symbol, code] : code_table) {
        auto* current = &root;
        for(const auto& position : code) {
            if(position == '0') {
                if(!current->left) current->left = new Node{};
                current = current->left;
            }

            if(position == '1') {
                if(!current->right) current->right = new Node{};
                current = current->right;
            }
        }
        current->symbol = symbol;
    }

    return root;
}

std::string Decoder::decode_content(const Node& root) {
    std::string output;
    auto current = root;

    while(true) {
        const auto byte = next_byte().to_string();
        for(size_t index = 0; index < byte.size(); index++) {
            const auto bit = byte.at(index);

            if(bit == '0') current = *current.left;
            if(bit == '1') current = *current.right;

            if(current.symbol) {
                if(current.symbol == END_OF_TEXT) return output;
                output += current.symbol;
                current = root;
            }
        }
    }

    return output;
}

void Decoder::create_decompressed_file() {
    const auto name = target_file_path.substr(0, target_file_path.find_last_of('.'));
    std::ofstream output_file(name, std::ios::out);

    const auto bits_length = decode_bits_length();
    const auto codes_length = decode_codes_length(bits_length);
    const auto codes = regenerate_codes(codes_length);
    const auto tree = recreate_huffman_tree(codes);
    const auto content = decode_content(tree);

    output_file.write(content.c_str(), content.size());

    output_file.close();
    file.close();
}
