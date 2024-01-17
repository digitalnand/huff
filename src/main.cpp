#include <algorithm>
#include <bitset>
#include <cmath>
#include <cstdint>
#include <format>
#include <fstream>
#include <iostream>
#include <map>
#include <queue>
#include <stack>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#define SUPPORTED_CHARACTERS 95
#define MAX_BITS (sizeof(int32_t) * 8)

template<typename T>
using min_priority_queue = std::priority_queue<T, std::vector<T>, std::greater<T>>;

struct huff_node {
    char symbol = '\0';
    uint32_t frequency = 0;

    huff_node* left = nullptr;
    huff_node* right = nullptr;

    friend bool operator>(const huff_node& left, const huff_node& right) {
        return left.frequency > right.frequency;
    }
};

std::unordered_map<char, uint32_t> extract_frequencies(std::ifstream& input) {
    std::unordered_map<char, uint32_t> frequencies(SUPPORTED_CHARACTERS);

    std::string line;
    while(std::getline(input, line)) {
        for(const char& character : line) {
            if(character - 32 > SUPPORTED_CHARACTERS)
                throw std::invalid_argument(std::format("unsupported character: {}\n", character));
            frequencies[character]++;
        }
    }

    return frequencies;
}

huff_node build_huffman_tree(const std::unordered_map<char, uint32_t>& frequencies) {
    min_priority_queue<huff_node> nodes;

    for(const auto& [symbol, frequency] : frequencies)
        nodes.push(huff_node{symbol, frequency});

    while(nodes.size() > 1) {
        const huff_node first = nodes.top();
        nodes.pop();
        const huff_node second = nodes.top();
        nodes.pop();

        huff_node parent;
        parent.frequency = first.frequency + second.frequency;
        parent.left = new huff_node{first};
        parent.right = new huff_node{second};

        nodes.push(parent);
    }

    return nodes.top();
}

std::vector<std::pair<char, std::string>> generate_huffman_codes(const huff_node& root) {
    std::vector<std::pair<char, std::string>> huffman_codes;

    std::stack<std::pair<huff_node, std::string>> nodes;
    nodes.push(std::make_pair(root, std::string{}));

    while(!nodes.empty()) {
        const huff_node current = nodes.top().first;
        std::string code = nodes.top().second;
        nodes.pop();

        if(current.left)
            nodes.push(std::make_pair(*current.left, code + '0'));

        if(current.right)
            nodes.push(std::make_pair(*current.right, code + '1'));

        if(current.symbol != '\0')
            huffman_codes.emplace_back(current.symbol, code);
    }

    std::sort(huffman_codes.begin(), huffman_codes.end(), [](const auto& left, const auto& right) {
        const char left_symbol = left.first;
        const char right_symbol = right.first;
        const size_t left_size = left.second.size();
        const size_t right_size = right.second.size();
        return (left_size == right_size) ? (left_symbol < right_symbol) : (left_size < right_size);
    });

    return huffman_codes;
}

std::string next_binary(std::string number) {
    size_t carry = 0;

    auto flip = [](const char& bit) -> char {
        return bit == '0' ? '1' : '0';
    };

    for(size_t index = 1; index <= (1 + carry); index++) {
        if(carry && index > number.size()) {
            number = '1' + number;
            carry--;
            continue;
        }
        char& last = number[number.size() - index];
        if(last == '1') carry++;
        last = flip(last);
    }

    return number;
}

std::unordered_map<char, std::string> generate_canonical_codes(const std::vector<std::pair<char, std::string>>& huffman_codes) {
    std::unordered_map<char, std::string> canonical_codes;

    const std::pair<char, std::string>& front_element = huffman_codes.front();
    for(size_t index = 0; index < front_element.second.size(); index++)
        canonical_codes[front_element.first] += '0';

    std::string last_code = canonical_codes[front_element.first];
    for(size_t index = 1; index < huffman_codes.size(); index++) {
        const std::pair<char, std::string>& current_element = huffman_codes.at(index);

        std::string current_code = next_binary(last_code);
        while(current_code.size() < huffman_codes.at(index).second.size()) current_code += '0';

        last_code = current_code;
        canonical_codes[current_element.first] = current_code;
    }

    return canonical_codes;
}

std::vector<char> encode_codes_length(std::unordered_map<char, std::string>& code_table) {
    std::vector<char> output;
    std::string buffer;

    uint32_t largest_code_length = 0;
    for(const auto& [symbol, code] : code_table) {
        if(code.size() < largest_code_length) continue;
        largest_code_length = code.size();
    }
    const uint32_t bit_count = std::log2(largest_code_length) + 1;

    output.push_back(bit_count);

    for(char character = 32; character < 127; character++) {
        const uint32_t code_length = code_table[character].size();
        std::string binary = std::bitset<MAX_BITS>(code_length).to_string().substr(MAX_BITS - bit_count);

        for(const char& bit : binary) {
            buffer += bit;
            if(buffer.size() % 8 != 0) continue;
            output.push_back(std::stoi(buffer, nullptr, 2));
            buffer.clear();
        }
    }

    if(buffer.size() % 8 != 0) {
        while(buffer.size() % 8 != 0) buffer += '0';
        output.push_back(std::stoi(buffer, nullptr, 2));
    }

    return output;
}

std::vector<char> encode_content(std::ifstream& file, std::unordered_map<char, std::string>& code_table) {
    std::vector<char> output;

    std::string current_line;
    std::string buffer;

    while(std::getline(file, current_line)) {
        for(const char& character : current_line) {
            const std::string code = code_table[character];
            for(size_t index = 0; index < code.size(); index++) {
                buffer += code.at(index);
                if(buffer.size() % 8 != 0) continue;
                output.push_back(std::stoi(buffer, nullptr, 2));
                buffer.clear();
            }
        }
    }

    if(buffer.size() % 8 != 0) {
        while(buffer.size() % 8 != 0) buffer += '0';
        output.push_back(std::stoi(buffer, nullptr, 2));
    }

    return output;
}


void create_compressed_file(const std::string& file_path) {
    std::ofstream output_file(std::format("{}.hf", file_path), std::ios::binary | std::ios::out);
    std::ifstream input_file(file_path);

    const std::unordered_map<char, uint32_t> frequencies = extract_frequencies(input_file);
    const huff_node tree = build_huffman_tree(frequencies);

    const std::vector<std::pair<char, std::string>> huffman_codes = generate_huffman_codes(tree);
    std::unordered_map<char, std::string> canonical_codes = generate_canonical_codes(huffman_codes);

    input_file.clear();
    input_file.seekg(0, input_file.beg);

    const std::vector<char> encoded_codes = encode_codes_length(canonical_codes);
    const std::vector<char> encoded_content = encode_content(input_file, canonical_codes);

    for(const char& bit : encoded_codes) output_file.put(bit);
    for(const char& bit : encoded_content) output_file.put(bit);

    input_file.close();
    output_file.close();
}

struct Decoder {
    std::ifstream file;

    size_t bits_length = 0;
    size_t index = 0;

    Decoder(const std::string&);
    std::bitset<8> next_byte();
    void decode_bits_length();
    std::vector<std::pair<char, uint16_t>> decode_codes_length();
    std::unordered_map<char, uint8_t> generate_codes();
};

Decoder::Decoder(const std::string& compressed_file_path) {
    file.open(compressed_file_path, std::ios::binary);
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
    char current = 32 - 1;

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

            current++;
            const uint16_t code_length = std::bitset<8>(binary_length).to_ulong();
            if(code_length > 0) codes_length.emplace_back(current, code_length);
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

std::unordered_map<char, uint8_t> Decoder::generate_codes() {
    std::unordered_map<char, uint8_t> canonical_codes;
    const auto codes_length = decode_codes_length();

    const auto& [front_symbol, front_length] = codes_length.front();

    uint8_t last_code = 0;
    auto last_length = front_length;

    canonical_codes[front_symbol] = last_code;

    for(size_t index = 1; index < codes_length.size(); index++) {
        const auto& [next_symbol, next_length] = codes_length.at(index);
        canonical_codes[next_symbol] = (last_code + 1) << (next_length - last_length);

        last_code = canonical_codes[next_symbol];
        last_length = next_length;
    }

    return canonical_codes;
}

int32_t main(int32_t argc, char* argv[]) {
    if(argc < 2) {
        std::cout << std::format("Usage: {} [FILE]\n", argv[0]);
        exit(1);
    }

    const std::string file_path = argv[1];
    create_compressed_file(file_path);
    return 0;
}
