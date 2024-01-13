#include <cstdint>
#include <format>
#include <iostream>
#include <map>
#include <queue>
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

auto extract_frequencies(const std::string_view input) -> min_priority_queue<Node> {
    min_priority_queue<Node> frequencies;
    std::map<char, uint32_t> symbol_table{};

    for(const char& character : input) {
        auto& frequency = symbol_table[character];
        frequency++;
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

auto main() -> int32_t {
    std::string input = "hello world";

    auto frequencies = extract_frequencies(input);
    auto tree = create_huffman_tree(frequencies);
    auto huffman_codes = generate_huffman_codes(tree);

    for(const auto& [symbol, code] : huffman_codes) {
        std::cout << std::format("symbol: {} | code: {}\n", symbol, code);
    }

    while(!frequencies.empty()) {
        const auto node = frequencies.top();
        std::cout << std::format("symbol: {} | frequency: {}\n", node.symbol, node.frequency);
        frequencies.pop();
    }

    return 0;
}
