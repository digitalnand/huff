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

auto analyze_frequency(const std::string_view input) -> std::vector<Node> {
    std::vector<Node> node_list;
    std::map<char, Node> symbols_lookup{};

    for(const char& character : input) {
        auto& node = symbols_lookup[character];
        node.symbol = character;
        node.frequency++;
    }

    for (const auto& [_, node] : symbols_lookup) {
        node_list.push_back(node);
    }

    return node_list;
}

auto main() -> int32_t {
    std::string input = "hello world";

    const auto node_list = analyze_frequency(input);

    for(const auto& node : node_list) {
        std::cout << std::format("symbol: {} | frequency: {}\n", node.symbol, node.frequency);
    }

    return 0;
}
