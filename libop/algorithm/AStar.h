#pragma once

#include <cmath>
#include <limits>
#include <list>
#include <queue>
#include <vector>

namespace op {

class AStar {
  public:
    struct Vec2i {
        int x = 0;
        int y = 0;

        bool operator==(const Vec2i &rhs) const {
            return x == rhs.x && y == rhs.y;
        }

        Vec2i operator+(const Vec2i &rhs) const {
            return {x + rhs.x, y + rhs.y};
        }
    };

    // 有效地图范围是 [0, w) x [0, h)，越界障碍点会被忽略。
    void set_map(int w, int h, const std::vector<Vec2i> &wall) {
        _mapSize = {w, h};
        _walls.assign(cell_count(), false);

        for (const auto &pos : wall) {
            if (!outside(pos)) {
                _walls[index(pos)] = true;
            }
        }
    }

    // 保持现有输出顺序：终点 -> 起点。
    void findpath(int beginX, int beginY, int endX, int endY, std::list<Vec2i> &path) {
        path.clear();

        const Vec2i start{beginX, beginY};
        const Vec2i end{endX, endY};
        if (outside(start) || outside(end) || blocked(start) || blocked(end)) {
            return;
        }

        // 每个格子的状态按 y * width + x 映射到一维数组。
        const int total = cell_count();
        std::vector<int> g_score(total, kUnreached);
        std::vector<int> parents(total, -1);
        std::vector<bool> closed(total, false);

        // 优先扩展预估总代价最低的节点。
        std::priority_queue<OpenNode, std::vector<OpenNode>, OpenNodeGreater> open;
        const int start_index = index(start);
        const int end_index = index(end);
        g_score[start_index] = 0;
        open.push({start, heuristic(start, end), 0});

        while (!open.empty()) {
            const OpenNode current = open.top();
            open.pop();

            const int current_index = index(current.pos);
            if (closed[current_index]) {
                continue;
            }
            closed[current_index] = true;

            if (current_index == end_index) {
                build_path(end_index, parents, path);
                return;
            }

            for (const auto &dir : kDirections) {
                const Vec2i next = current.pos + dir;
                if (outside(next) || blocked(next)) {
                    continue;
                }

                const int next_index = index(next);
                if (closed[next_index]) {
                    continue;
                }

                const int next_g = g_score[current_index] + 1;
                if (next_g >= g_score[next_index]) {
                    continue;
                }

                g_score[next_index] = next_g;
                parents[next_index] = current_index;
                open.push({next, next_g + heuristic(next, end), next_g});
            }
        }
    }

  private:
    struct OpenNode {
        Vec2i pos;
        int f = 0;
        int g = 0;
    };

    struct OpenNodeGreater {
        bool operator()(const OpenNode &lhs, const OpenNode &rhs) const {
            if (lhs.f != rhs.f) {
                return lhs.f > rhs.f;
            }
            return lhs.g < rhs.g;
        }
    };

    static constexpr int kUnreached = (std::numeric_limits<int>::max)();
    // 八方向移动，每步代价相同。
    static constexpr Vec2i kDirections[8] = {
        {0, 1},
        {0, -1},
        {-1, 0},
        {1, 0},
        {1, 1},
        {-1, 1},
        {-1, -1},
        {1, -1},
    };

    bool outside(Vec2i pos) const {
        return pos.x < 0 || pos.y < 0 || pos.x >= _mapSize.x || pos.y >= _mapSize.y;
    }

    bool blocked(Vec2i pos) const {
        return _walls.empty() || _walls[index(pos)];
    }

    int cell_count() const {
        if (_mapSize.x <= 0 || _mapSize.y <= 0) {
            return 0;
        }
        return _mapSize.x * _mapSize.y;
    }

    int index(Vec2i pos) const {
        return pos.y * _mapSize.x + pos.x;
    }

    // 八方向等代价移动适合使用切比雪夫距离。
    int heuristic(Vec2i from, Vec2i to) const {
        const int dx = std::abs(from.x - to.x);
        const int dy = std::abs(from.y - to.y);
        return dx > dy ? dx : dy;
    }

    // 命中终点后，通过父节点索引回溯路径。
    void build_path(int end_index, const std::vector<int> &parents, std::list<Vec2i> &path) const {
        for (int current = end_index; current >= 0; current = parents[current]) {
            path.push_back({current % _mapSize.x, current / _mapSize.x});
        }
    }

    Vec2i _mapSize;
    std::vector<bool> _walls;
};

} // namespace op
