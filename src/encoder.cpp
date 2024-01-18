#include <algorithm>
#include <bitset>
#include <cmath>
#include <format>
#include <stack>

#include "encoder.hpp"

std::unordered_map<char, uint32_t> extract_frequencies(std::ifstream& input) {
    std::unordered_map<char, uint32_t> frequencies(SUPPORTED_CHARACTERS);
    std::string line;

    while(std::getline(input, line)) {
        for(const auto& character : line + NEW_LINE) {
            if(character < FIRST_CHARACTER)
                throw std::invalid_argument(std::format("unsupported character: {}\n", character));
            frequencies[character]++;
        }
    }

    frequencies[END_OF_TEXT]++;

    return frequencies;
}

Node build_huffman_tree(const std::unordered_map<char, uint32_t>& frequencies) {
    min_priority_queue<Node> nodes;

    for(const auto& [symbol, frequency] : frequencies)
        nodes.push(Node{symbol, frequency});

    while(nodes.size() > 1) {
        const auto first = nodes.top();
        nodes.pop();
        const auto second = nodes.top();
        nodes.pop();

        Node parent;
        parent.frequency = first.frequency + second.frequency;
        parent.left = new Node{first};
        parent.right = new Node{second};

        nodes.push(parent);
    }

    return nodes.top();
}

std::vector<std::pair<char, std::string>> generate_huffman_codes(const Node& root) {
    std::vector<std::pair<char, std::string>> huffman_codes;

    std::stack<std::pair<Node, std::string>> nodes;
    nodes.push(std::make_pair(root, std::string{}));

    while(!nodes.empty()) {
        const auto [top_node, top_code] = nodes.top();
        nodes.pop();

        if(top_node.left)
            nodes.push(std::make_pair(*top_node.left, top_code + '0'));

        if(top_node.right)
            nodes.push(std::make_pair(*top_node.right, top_code + '1'));

        if(top_node.symbol != '\0')
            huffman_codes.emplace_back(top_node.symbol, top_code);
    }

    std::sort(huffman_codes.begin(), huffman_codes.end(), [](const auto& left, const auto& right) {
        const auto& [left_symbol, left_code] = left;
        const auto& [right_symbol, right_code] = right;
        return (left_code.size() == right_code.size()) ?
                (left_symbol < right_symbol) : (left_code.size() < right_code.size());
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

    const auto& [front_symbol, front_code] = huffman_codes.front();
    for(size_t index = 0; index < front_code.size(); index++)
        canonical_codes[front_symbol] += '0';

    auto last_code = canonical_codes[front_symbol];
    for(size_t index = 1; index < huffman_codes.size(); index++) {
        const auto& [current_symbol, current_code] = huffman_codes.at(index);

        auto next_code = next_binary(last_code);
        while(next_code.size() < current_code.size()) next_code += '0';

        last_code = next_code;
        canonical_codes[current_symbol] = next_code;
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

    for(char character = FIRST_CHARACTER; character < SUPPORTED_CHARACTERS; character++) {
        const uint32_t code_length = code_table[character].size();
        std::string binary = std::bitset<MAX_BITS>(code_length).to_string().substr(MAX_BITS - bit_count);

        for(const auto& bit : binary) {
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

    std::string buffer;
    std::string line;

    while(std::getline(file, line)) {
        for(const auto& character : line + NEW_LINE) {
            const auto code = code_table[character];
            for(size_t index = 0; index < code.size(); index++) {
                buffer += code.at(index);
                if(buffer.size() % 8 != 0) continue;
                output.push_back(std::stoi(buffer, nullptr, 2));
                buffer.clear();
            }
        }
    }

    for(size_t index = 0; index < code_table[END_OF_TEXT].size(); index++) {
        buffer += code_table[END_OF_TEXT].at(index);
        if(buffer.size() % 8 != 0) continue;
        output.push_back(std::stoi(buffer, nullptr, 2));
        buffer.clear();
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

    const auto frequencies = extract_frequencies(input_file);
    const auto tree = build_huffman_tree(frequencies);

    const auto huffman_codes = generate_huffman_codes(tree);
    auto canonical_codes = generate_canonical_codes(huffman_codes);

    input_file.clear();
    input_file.seekg(0, input_file.beg);

    const auto encoded_codes = encode_codes_length(canonical_codes);
    const auto encoded_content = encode_content(input_file, canonical_codes);

    for(const auto& bit : encoded_codes) output_file.put(bit);
    for(const auto& bit : encoded_content) output_file.put(bit);

    input_file.close();
    output_file.close();
}
