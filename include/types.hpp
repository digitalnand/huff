#pragma once

#include <cstdint>
#include <queue>

#define FIRST_CHARACTER 0
#define SUPPORTED_CHARACTERS 127
#define MAX_BITS (sizeof(uint32_t) * 8)

#define NEW_LINE '\n'
#define END_OF_TEXT '\x3'

template<typename T>
using min_priority_queue = std::priority_queue<T, std::vector<T>, std::greater<T>>;

struct Node {
    char symbol = '\0';
    uint32_t frequency = 0;

    Node* left = nullptr;
    Node* right = nullptr;

    friend bool operator>(const Node& left, const Node& right) {
        return left.frequency > right.frequency;
    }
};
