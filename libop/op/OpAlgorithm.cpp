#include "OpContext.h"

#include "algorithm/AStar.h"
#include "runtime/RuntimeUtils.h"

#include <libop.h>

#include <cwchar>
#include <list>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

void op::Op::AStarFindPath(long mapWidth, long mapHeight, const wchar_t *disable_points, long beginX, long beginY,
                          long endX, long endY, std::wstring &path) {
    AStar as;
    using Vec2i = AStar::Vec2i;
    std::vector<Vec2i> walls;
    std::vector<wstring> vstr;
    Vec2i tp;
    split(disable_points, vstr, L"|");
    for (auto &it : vstr) {
        if (swscanf(it.c_str(), L"%d,%d", &tp.x, &tp.y) != 2)
            break;
        walls.push_back(tp);
    }
    std::list<Vec2i> paths;

    as.set_map(mapWidth, mapHeight, walls);
    as.findpath(beginX, beginY, endX, endY, paths);
    path.clear();
    for (auto it = paths.rbegin(); it != paths.rend(); ++it) {
        auto v = *it;
        path += std::to_wstring(v.x);
        path.push_back(L',');
        path += std::to_wstring(v.y);
        path.push_back(L'|');
    }
    if (!path.empty())
        path.pop_back();
}

void op::Op::FindNearestPos(const wchar_t *all_pos, long type, long x, long y, std::wstring &ret) {
    double old = 1e9;
    long rx = -1, ry = -1;
    std::wstring best_name;
    std::wstring s = std::regex_replace(all_pos, std::wregex(L","), L" ");
    std::vector<std::wstring> items;
    split(s, items, L"|");
    for (const auto &item : items) {
        long x2, y2;
        bool ok = false;
        std::wstring name;
        std::wistringstream iss(item);
        if (type == 1) {
            if (iss >> x2 >> y2) {
                ok = true;
            }
        } else {
            if (iss >> name >> x2 >> y2) {
                ok = true;
            }
        }
        if (ok) {
            double compareDis = (x - x2) * (x - x2) + (y - y2) * (y - y2);
            if (compareDis < old) {
                rx = x2;
                ry = y2;
                old = compareDis;
                best_name = name;
            }
        }
    }
    if (!best_name.empty()) {
        ret = best_name + L"," + std::to_wstring(rx) + L"," + std::to_wstring(ry);
    } else if (type == 1 && rx != -1) {
        ret = std::to_wstring(rx) + L"," + std::to_wstring(ry);
    } else {
        ret.clear();
    }
}

