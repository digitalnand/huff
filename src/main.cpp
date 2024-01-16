#include <algorithm>
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
                throw std::invalid_argument(std::format("unsupported character: {}\n", static_cast<char>(character)));
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
    for(size_t index = 0; index < front_element.second.size(); index++) {
        canonical_codes[front_element.first] += '0';
    }

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
                output.push_back(static_cast<char>(std::stoi(buffer, nullptr, 2)));
                buffer.clear();
            }
        }
    }

    if(buffer.size() % 8 != 0) {
        while(buffer.size() % 8 != 0) buffer += '0';
        output.push_back(static_cast<char>(std::stoi(buffer, nullptr, 2)));
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

    const std::vector<char> encoded_content = encode_content(input_file, canonical_codes);

    for(const char& bit : encoded_content) {
        output_file.put(bit);
    }

    input_file.close();
    output_file.close();
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
