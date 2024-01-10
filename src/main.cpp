#include <cstdint>
#include <format>
#include <iostream>
#include <map>
#include <string_view>
#include <vector>

struct Node {
    char symbol;
    uint32_t frequency = 0;
};

auto extract_frequencies(const std::string_view input) -> std::vector<Node> {
    std::vector<Node> node_list;
    std::map<char, Node> symbol_lookup{};

    for(const char& character : input) {
        auto& node = symbol_lookup[character];
        node.symbol = character;
        node.frequency++;
    }

    for(const auto& [_, node] : symbol_lookup) {
        node_list.push_back(node);
    }

    return node_list;
}

auto main() -> int32_t {
    std::string input = "hello world";

    const auto node_list = extract_frequencies(input);

    for(const auto& node : node_list) {
        std::cout << std::format("symbol: {} | frequency: {}\n", node.symbol, node.frequency);
    }

    return 0;
}
