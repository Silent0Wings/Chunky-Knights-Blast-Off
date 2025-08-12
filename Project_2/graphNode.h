#pragma once
#include <array>
#include <tuple>
#include <vector>

struct graphNode
{
    int value = 0;
    std::vector<std::tuple<graphNode *, size_t>> children; // neighbor + weight
    graphNode *parent = nullptr;
    std::array<size_t, 2> index = {0, 0};

    bool blocked = false;
    bool explored = false;
    bool goal = false;

    graphNode() = default;
};
