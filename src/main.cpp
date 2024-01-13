#include <cstdint>
#include <format>
#include <fstream>
#include <getopt.h>
#include <iostream>
#include <map>
#include <queue>
#include <string>
#include <string_view>
#include <vector>

template <typename T>
using min_priority_queue = std::priority_queue<T, std::vector<T>, std::greater<T>>;

struct Node {
    char symbol;
    uint32_t frequency = 0;

    Node* left;
    Node* right;

    friend bool operator>(const Node& l, const Node& r) {
        return l.frequency > r.frequency;
    }

    friend bool operator<(const Node& l, const Node& r) {
        return l.frequency < r.frequency;
    }
};

auto extract_frequencies(const std::string& file_path) -> min_priority_queue<Node> {
    std::ifstream file(file_path);
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
    if(root.left) {
        codes = generate_huffman_codes(*root.left, current_code + '0', codes);
    }

    if(root.symbol) {
        codes[root.symbol] = current_code;
        current_code = "";
    }

    if(root.right) {
        codes = generate_huffman_codes(*root.right, current_code + '1', codes);
    }

    return codes;
}

auto encode_data(const std::string& file_path) -> std::string {
    std::string output;
    std::ifstream file(file_path);
    std::string current_line;

    const auto frequencies = extract_frequencies(file_path);
    const auto tree = create_huffman_tree(frequencies);
    auto code_table = generate_huffman_codes(tree);

    while(std::getline(file, current_line)) {
        for(const char& character : current_line) {
            output += code_table[character];
        }
    }

    return output;
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
                std::cout << std::format("Usage: {} [OPTIONS] INPUT\n\n", argv[0]);
                std::cout << "Options:\n";
                for(size_t index = 0; index < (sizeof(long_options) / sizeof(option)) - 1; index++) {
                    const auto& current_option = long_options[index];

                    std::cout << std::format("\t-{}, --{}", (char) current_option.val, current_option.name);
                    if(current_option.has_arg == required_argument) std::cout << " [argument]";

                    std::cout << std::endl;
                }
                break;
            case 'c': {
                const auto file_path = std::string(optarg);
                const auto output = encode_data(file_path);
                std::cout << output << std::endl;
                break;
            }
        }
    }
    return 0;
}
