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

void Decoder::decode_bits_length() {
    bits_length = next_byte().to_ulong();
}

std::vector<std::pair<char, uint16_t>> Decoder::decode_codes_length() {
    decode_bits_length();

    std::vector<std::pair<char, uint16_t>> codes_length;

    std::string carry;
    char current = FIRST_CHARACTER;

    while(index <= std::ceil(SUPPORTED_CHARACTERS / 8.0) * bits_length) {
        std::string byte = next_byte().to_string();

        if(!carry.empty()) {
            byte = carry + byte;
            carry.clear();
        }

        for(size_t position = 0; position < byte.size(); position += bits_length) {
            const std::string binary_length = byte.substr(position, bits_length);

            if(binary_length.size() < bits_length) {
                carry = binary_length;
                break;
            }

            const uint16_t code_length = std::bitset<8>(binary_length).to_ulong();
            if(code_length > 0) codes_length.emplace_back(current, code_length);
            current++;
        }
    }

    std::sort(codes_length.begin(), codes_length.end(), [](const auto& left, const auto& right) {
        const auto left_symbol = left.first;
        const auto right_symbol = right.first;
        const auto left_length = left.second;
        const auto right_length = right.second;
        return (left_length == right_length) ? (left_symbol < right_symbol) : (left_length < right_length);
    });

    return codes_length;
}

std::unordered_map<char, std::string> Decoder::generate_codes() {
    std::unordered_map<char, std::string> canonical_codes;
    const auto codes_length = decode_codes_length();

    const auto& [front_symbol, front_length] = codes_length.front();

    for(size_t index = 0 ; index < front_length; index++)
        canonical_codes[front_symbol] += '0';

    uint8_t last_code = 0;
    auto last_length = front_length;

    for(size_t index = 1; index < codes_length.size(); index++) {
        const auto& [next_symbol, next_length] = codes_length.at(index);

        last_code = (last_code + 1) << (next_length - last_length);;
        last_length = next_length;

        canonical_codes[next_symbol] = std::bitset<MAX_BITS>(last_code).to_string().substr(MAX_BITS - next_length);
    }

    return canonical_codes;
}

Node Decoder::recreate_huffman_tree() {
    Node root{};

    const auto code_table = generate_codes();

    for(const auto& [symbol, code] : code_table) {
        Node* current = &root;
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

std::string Decoder::decode_content() {
    std::string output;
    const auto tree = recreate_huffman_tree();

    std::string byte = next_byte().to_string();

    Node current = tree;
    for(size_t index = 0; index < byte.size(); index++) {
        const auto bit = byte.at(index);

        if(bit == '0') current = *current.left;
        if(bit == '1') current = *current.right;

        if(current.symbol) {
            if(current.symbol == END_OF_TEXT) break;
            output += current.symbol;
            current = tree;
        }

        if((index + 1) % 8 == 0) {
            byte = next_byte().to_string();
            index = -1;
        }
    }

    return output;
}

void Decoder::create_decompressed_file() {
    std::string name = target_file_path.substr(0, target_file_path.find_last_of('.'));
    std::ofstream output_file(name, std::ios::out);

    const auto content = decode_content();
    output_file.write(content.c_str(), content.size());

    output_file.close();
    file.close();
}
