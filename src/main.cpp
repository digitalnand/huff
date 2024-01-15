#include <algorithm>
#include <bitset>
#include <cstdint>
#include <format>
#include <fstream>
#include <getopt.h>
#include <iostream>
#include <map>
#include <queue>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

template <typename T>
using min_priority_queue = std::priority_queue<T, std::vector<T>, std::greater<T>>;

struct Node {
    char symbol = '\0';
    uint32_t frequency = 0;

    Node* left;
    Node* right;

    friend bool operator>(const Node& l, const Node& r) {
        return l.frequency > r.frequency;
    }
};

auto extract_frequencies(std::ifstream& file) -> min_priority_queue<Node> {
    file.clear();
    file.seekg(0, file.beg);

    min_priority_queue<Node> frequencies;
    std::map<char, uint32_t> symbol_table{};
    std::string current_line;

    while(std::getline(file, current_line)) {
        for(const char& character : current_line) {
            auto& frequency = symbol_table[character];
            frequency++;
        }
    }

    for(const auto& [symbol, frequency] : symbol_table) {
        frequencies.push(Node{symbol, frequency, nullptr, nullptr});
    }

    return frequencies;
}

auto create_huffman_tree(min_priority_queue<Node> frequencies) -> Node {
    while(frequencies.size() > 1) {
        const Node first = frequencies.top(); frequencies.pop();
        const Node second = frequencies.top(); frequencies.pop();

        Node parent;
        parent.frequency = first.frequency + second.frequency;
        parent.left = new Node{first};
        parent.right = new Node{second};

        frequencies.push(parent);
    }

    return frequencies.top();
}

auto generate_huffman_codes(const Node& root, std::string current_code = "",
                           std::map<char, std::string> codes = {}) -> std::map<char, std::string> {
    if(root.left)
        codes = generate_huffman_codes(*root.left, current_code + '0', codes);

    if(root.right)
        codes = generate_huffman_codes(*root.right, current_code + '1', codes);

    if(root.symbol != '\0') {
        codes[root.symbol] = current_code;
        current_code.clear();
    }

    return codes;
}

auto sort_huffman_codes(std::map<char, std::string> huffman_codes) -> std::vector<std::pair<char, std::string>> {
    std::vector<std::pair<char, std::string>> sorted_codes;

    for(const auto& [symbol, code] : huffman_codes) {
        sorted_codes.emplace_back(symbol, code);
    }

    std::sort(sorted_codes.begin(), sorted_codes.end(), [](const auto& left, const auto& right) {
        return left.second.size() < right.second.size();
    });

    return sorted_codes;
}

auto next_binary(std::string number) -> std::string {
    size_t next = 1;
    size_t carry = 0;

    auto flip = [](const char& bit) {
        return bit == '0' ? '1' : '0';
    };

    for(size_t index = 1; index <= (next + carry); index++) {
        if(carry && index > number.size()) {
            number = '1' + number;
            carry--;
            continue;
        }
        auto& last = number[number.size() - index];
        if(last == '1') carry++;
        last = flip(last);
    }

    return number;
}

auto generate_canonical_codes(std::map<char, std::string>& huffman_codes) {
    std::map<char, std::string> canonical_codes;
    const auto sorted_codes = sort_huffman_codes(huffman_codes);

    const auto& front_element = sorted_codes.front();
    for(size_t index = 0; index < front_element.second.size(); index++) {
        canonical_codes[front_element.first] += "0";
    }

    auto last_code = canonical_codes[front_element.first];
    for(size_t index = 1; index < huffman_codes.size(); index++) {
        const auto& current_element = sorted_codes.at(index);

        auto current_code = next_binary(last_code);
        while(current_code.size() < sorted_codes.at(index).second.size()) current_code += '0';

        last_code = current_code;
        canonical_codes[current_element.first] = current_code;
    }

    return canonical_codes;
}

auto encode_content(std::ifstream& file, std::map<char, std::string>& code_table) -> std::vector<char> {
    file.clear();
    file.seekg(0, file.beg);

    std::vector<char> output;

    std::string current_line;
    std::string buffer;

    while(std::getline(file, current_line)) {
        for(const char& character : current_line) {
            const auto code = code_table[character];
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

auto create_compressed_file(const std::string& file_path) {
    std::ofstream output_file(std::format("{}.hf", file_path), std::ios::binary | std::ios::out);
    std::ifstream input_file(file_path);

    const auto frequencies = extract_frequencies(input_file);
    const auto tree = create_huffman_tree(frequencies);

    auto huffman_codes = generate_huffman_codes(tree);
    auto canonical_codes = generate_canonical_codes(huffman_codes);

    const auto encoded_content = encode_content(input_file, canonical_codes);

    for(const auto& bit : encoded_content) {
        output_file.put(bit);
    }

    input_file.close();
    output_file.close();
}

auto decode_data(const std::string_view coded_data, const Node& tree) -> std::string {
    std::string output;
    Node current_node = tree;

    for(char character : coded_data) {    
        if(character == '0') current_node = *current_node.left;
        if(character == '1') current_node = *current_node.right;

        if(current_node.symbol) {
            output += current_node.symbol;
            current_node = tree;
        }
    }

    return output;
}

auto print_help(char* executable_name, struct option* options, size_t options_size) -> void {
    std::cout << std::format("Usage: {} [OPTIONS] INPUT\n\n", executable_name);
    std::cout << "Options:\n";
    for(size_t index = 0; index < options_size; index++) {
        const auto& current_option = options[index];

        std::cout << std::format("\t-{}, --{}", (char) current_option.val, current_option.name);
        if(current_option.has_arg == required_argument) std::cout << " [argument]";

        std::cout << "\n";
    }
}

auto main(int32_t argc, char** argv) -> int32_t {
    static struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {"compress", required_argument, 0, 'c'},
        {0, 0, 0, 0}
    };

    int32_t opt;
    int32_t option_index;

    while((opt = getopt_long(argc, argv, "hc:", long_options, &option_index)) != -1) {
        switch(opt) {
            case 'h':
                print_help(argv[0], long_options, sizeof(long_options) / sizeof(struct option) - 1);
                break;
            case 'c': {
                const auto file_path = std::string(optarg);
                create_compressed_file(file_path);
                break;
            }
        }
    }
    return 0;
}
