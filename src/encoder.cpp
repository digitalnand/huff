#include <algorithm>
#include <bitset>
#include <cmath>
#include <format>
#include <stack>

#include "encoder.hpp"

Encoder::Encoder(const std::string& file_path) {
    const auto extension = file_path.substr(file_path.find_last_of("."));
    if(extension != ".txt")
        throw std::runtime_error(std::format("file is not .txt: {}\n", file_path));

    this->target.open(file_path);
    if(!this->target.is_open())
        throw std::runtime_error(std::format("could not open the file: {}\n", file_path));

    this->target_path = file_path;
}

std::unordered_map<char, uint32_t> Encoder::extract_frequencies() {
    std::unordered_map<char, uint32_t> frequencies(SUPPORTED_CHARACTERS);
    std::string line;

    this->target.clear();
    this->target.seekg(0, this->target.beg);

    while(std::getline(this->target, line)) {
        for(const auto& character : line + NEW_LINE) {
            if(character < FIRST_CHARACTER)
                throw std::invalid_argument(std::format("unsupported character: {}\n", character));
            frequencies[character]++;
        }
    }

    frequencies[END_OF_TEXT]++;

    return frequencies;
}

Node Encoder::build_huffman_tree(const std::unordered_map<char, uint32_t>& frequencies) {
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

std::vector<std::pair<char, uint32_t>> Encoder::get_huffman_codes_length(const Node& root) {
    std::vector<std::pair<char, uint32_t>> codes_length;

    std::stack<std::pair<Node, uint32_t>> nodes;
    nodes.push(std::make_pair(root, 0));

    while(!nodes.empty()) {
        const auto [top_node, top_length] = nodes.top();
        nodes.pop();

        if(top_node.left)
            nodes.push(std::make_pair(*top_node.left, top_length + 1));

        if(top_node.right)
            nodes.push(std::make_pair(*top_node.right, top_length + 1));

        if(top_node.symbol != '\0')
            codes_length.emplace_back(top_node.symbol, top_length);
    }

    std::sort(codes_length.begin(), codes_length.end(), [](const auto& left, const auto& right) {
        const auto& [left_symbol, left_length] = left;
        const auto& [right_symbol, right_length] = right;
        return (left_length == right_length) ? (left_symbol < right_symbol) : (left_length < right_length);
    });

    return codes_length;
}

std::unordered_map<char, std::string> Encoder::generate_canonical_codes(const std::vector<std::pair<char, uint32_t>>& codes_length) {
    std::unordered_map<char, std::string> codes;

    const auto& [front_symbol, front_length] = codes_length.front();
    for(size_t index = 0; index < front_length; index++)
        codes[front_symbol] += '0';

    uint32_t last_code = 0;
    auto last_length = front_length;

    for(size_t index = 1; index < codes_length.size(); index++) {
        const auto& [current_symbol, current_length] = codes_length.at(index);

        const auto difference = current_length - last_length;
        const auto next_code = (last_code + 1) << difference;

        codes[current_symbol] = std::bitset<MAX_BITS>(next_code).to_string().substr(MAX_BITS - current_length);

        last_code = next_code;
        last_length = current_length;
    }

    return codes;
}

inline void append_byte(std::string& buffer, std::vector<char>& target) {
    target.push_back(std::stoi(buffer, nullptr, 2));
    buffer.clear();
}

inline void fill(std::string& buffer) {
    while(buffer.size() % 8 != 0) buffer += '0';
}

std::vector<char> Encoder::encode_codes_length(const std::unordered_map<char, std::string>& code_table) {
    std::vector<char> output;
    std::string buffer;

    uint32_t longest_length = 0;
    for(const auto& [_, code] : code_table) {
        if(code.size() < longest_length) continue;
        longest_length = code.size();
    }

    const uint32_t bit_count = std::log2(longest_length) + 1;
    output.push_back(bit_count);

    for(char character = FIRST_CHARACTER; character < SUPPORTED_CHARACTERS; character++) {
        const uint32_t length = code_table.contains(character) ? code_table.at(character).size() : 0;
        std::string binary = std::bitset<MAX_BITS>(length).to_string().substr(MAX_BITS - bit_count);

        for(const auto& bit : binary) {
            buffer += bit;
            if(buffer.size() % 8 == 0) append_byte(buffer, output);
        }
    }

    if(buffer.size() % 8 != 0) {
        fill(buffer);
        append_byte(buffer, output);;
    }

    return output;
}

std::vector<char> Encoder::encode_content(const std::unordered_map<char, std::string>& code_table) {
    std::vector<char> output;

    std::string buffer;
    std::string line;

    this->target.clear();
    this->target.seekg(0, this->target.beg);

    while(std::getline(this->target, line)) {
        for(const auto& character : line + NEW_LINE) {
            const auto code = code_table.at(character);
            for(size_t index = 0; index < code.size(); index++) {
                buffer += code.at(index);
                if(buffer.size() % 8 == 0) append_byte(buffer, output);
            }
        }
    }

    const auto eot_code = code_table.at(END_OF_TEXT);
    for(size_t index = 0; index < eot_code.size(); index++) {
        buffer += eot_code.at(index);
        if(buffer.size() % 8 == 0) append_byte(buffer, output);
    }

    if(buffer.size() % 8 != 0) {
        fill(buffer);
        append_byte(buffer, output);;
    }

    return output;
}

void Encoder::create_compressed_file() {
    const auto frequencies = this->extract_frequencies();
    const auto tree = this->build_huffman_tree(frequencies);

    const auto codes_length = this->get_huffman_codes_length(tree);
    const auto codes = this->generate_canonical_codes(codes_length);

    const auto encoded_length = this->encode_codes_length(codes);
    const auto encoded_content = this->encode_content(codes);

    std::ofstream output(std::format("{}.hf", this->target_path), std::ios::binary | std::ios::out);

    for(const auto& bit : encoded_length) output.put(bit);
    for(const auto& bit : encoded_content) output.put(bit);

    output.close();
    this->target.close();
}
