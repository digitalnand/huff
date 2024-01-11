#include <cstdint>
#include <format>
#include <iostream>
#include <map>
#include <queue>
#include <string_view>
#include <vector>

struct Node {
    char symbol;
    uint32_t frequency = 0;

    friend bool operator>(const Node& l, const Node& r) {
        return l.frequency > r.frequency;
    }

    friend bool operator<(const Node& l, const Node& r) {
        return l.frequency < r.frequency;
    }
};

auto extract_frequencies(const std::string_view input) -> std::priority_queue<Node> {
    std::priority_queue<Node> nodes;
    std::map<char, uint32_t> symbol_table{};

    for(const char& character : input) {
        auto& frequency = symbol_table[character];
        frequency++;
    }

    for(const auto& [symbol, frequency] : symbol_table) {
        nodes.push(Node{symbol, frequency});
    }

    return nodes;
}

auto main() -> int32_t {
    std::string input = "hello world";

    auto nodes = extract_frequencies(input);

    while(!nodes.empty()) {
        const auto node = nodes.top();
        std::cout << std::format("symbol: {} | frequency: {}\n", node.symbol, node.frequency);
        nodes.pop();
    }

    return 0;
}
