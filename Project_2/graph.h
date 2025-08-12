#ifndef GRAPH_H
#define GRAPH_H

#include "graphNode.h"

#include <array>
#include <tuple>
#include <vector>
#include <queue>     // std::priority_queue (if you use it elsewhere)
#include <stdexcept> // std::invalid_argument
#include <cstdlib>   // std::rand
#include <random>    // optional RNG utilities

/**
 * @class graph
 * @brief describes a graph
 **/
class graph
{
public:
    graphNode *root = nullptr;
    graphNode *goal = nullptr;

    graph() = default;

    std::vector<std::vector<std::tuple<graphNode, std::array<int, 3>>>> gridNode;
    size_t Height = 0;
    size_t Width = 0;

    std::vector<std::vector<int>> directions = {
        {1, 0},  // down
        {-1, 0}, // up
        {0, 1},  // right
        {0, -1}  // left
    };

    std::vector<std::vector<int>> full_directions = {
        {1, 1},   // down-right
        {-1, -1}, // up-left
        {1, -1},  // down-left
        {-1, 1},  // up-right
        {1, 0},   // down
        {-1, 0},  // up
        {0, 1},   // right
        {0, -1}   // left
    };

    graph(const size_t height, const size_t width)
    {
        if (width == 0 || height == 0)
            throw std::invalid_argument("Width and height must be non-zero");

        Height = height;
        Width = width;

        int increment = 0;

        int incrementX = 0;
        int incrementY = 0;
        for (size_t i = 0; i < height; i++)
        {
            std::vector<std::tuple<graphNode, std::array<int, 3>>> temprow;
            temprow.reserve(width);

            for (size_t z = 0; z < width; z++)
            {
                graphNode tempNode;
                tempNode.value = increment++; // unique per cell
                tempNode.index = {i, z};

                temprow.emplace_back(tempNode, std::array<int, 3>{(int)i, 0, (int)z});
                incrementX++;
            }
            incrementY++;

            gridNode.push_back(std::move(temprow));
        }

        for (size_t i = 0; i < gridNode.size(); i++)
        {
            for (size_t z = 0; z < gridNode[i].size(); z++)
            {
                for (const auto &direct : directions)
                {
                    const size_t ni = i + direct[0];
                    const size_t nz = z + direct[1];

                    if (graphConstraing(static_cast<int>(ni), static_cast<int>(nz), height, Width))
                    {
                        size_t r = static_cast<size_t>(std::rand()) % 100; // 0–99
                        std::get<0>(gridNode[i][z]).children.emplace_back(&std::get<0>(gridNode[ni][nz]), r % 40);
                    }
                }
            }
        }

        root = &std::get<0>(gridNode[0][0]);
        root->explored = true;

        std::get<0>(gridNode[height - 1][width - 1]).goal = true;
        goal = &std::get<0>(gridNode[height - 1][width - 1]);
    }

    static bool graphConstraing(int x, int y, size_t height, size_t width)
    {
        return !(x < 0 || x >= static_cast<int>(height) || y < 0 || y >= static_cast<int>(width));
    }

    std::vector<std::array<int, 3>> getLocation()
    {
        std::vector<std::array<int, 3>> locations;

        for (const auto &row : gridNode)
        {
            for (const auto &cell : row)
            {
                const auto &obj = std::get<1>(cell); // the std::array<int, 3>
                locations.push_back(obj);
            }
        }

        return locations;
    }
};

#endif // GRAPH_H
