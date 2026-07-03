// #include "stdafx.h"
#include "ImageSearchAlgorithms.h"

#include <assert.h>
#include <time.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <numeric>
#include <queue>

#include "../runtime/RuntimeUtils.h"

using std::to_wstring;

namespace op::image {

namespace {

color_t sim_to_color_diff(double sim) {
    if (sim < 0.0 || sim > 1.0)
        sim = 1.0;

    const auto diff = static_cast<uchar>(std::ceil((1.0 - sim) * 255.0));
    color_t color_diff;
    color_diff.b = diff;
    color_diff.g = diff;
    color_diff.r = diff;
    return color_diff;
}

bool has_color_diff(const color_t &df) {
    return df.b != 0 || df.g != 0 || df.r != 0;
}

bool color_matches(const color_t &color, const color_df_t &expected, double sim) {
    const color_t df = has_color_diff(expected.df) ? expected.df : sim_to_color_diff(sim);
    return IN_RANGE(color, expected.color, df);
}

void apply_implicit_color_diff(std::vector<color_df_t> &colors, double sim) {
    const color_t implicit_df = sim_to_color_diff(sim);
    for (auto &it : colors) {
        if (!has_color_diff(it.df))
            it.df = implicit_df;
    }
}

std::vector<color_df_t> prepared_color_dfs(const std::vector<color_df_t> &colors, double sim) {
    std::vector<color_df_t> prepared = colors;
    apply_implicit_color_diff(prepared, sim);
    return prepared;
}

std::vector<pt_cr_df_t> prepared_point_color_dfs(const std::vector<pt_cr_df_t> &points, double sim) {
    std::vector<pt_cr_df_t> prepared = points;
    for (auto &it : prepared)
        apply_implicit_color_diff(it.crdfs, sim);
    return prepared;
}

bool color_matches_prepared(const color_t &color, const color_df_t &expected) {
    return IN_RANGE(color, expected.color, expected.df);
}

bool any_color_matches_prepared(const color_t &color, const std::vector<color_df_t> &colors) {
    for (const auto &it : colors) {
        if (color_matches_prepared(color, it))
            return true;
    }
    return false;
}

struct ocr_text_span_t {
    size_t begin = 0;
    size_t end = 0;
    point_t point;
};

std::wstring build_ocr_text_spans(const std::map<point_t, ocr_rec_t> &ps, std::vector<ocr_text_span_t> &spans) {
    spans.clear();
    std::wstring text;

    for (const auto &it : ps) {
        if (it.second.text.empty())
            continue;

        ocr_text_span_t span;
        span.begin = text.size();
        text.append(it.second.text);
        span.end = text.size();
        span.point = it.first;
        spans.push_back(span);
    }

    return text;
}

const ocr_text_span_t *find_ocr_text_span(const std::vector<ocr_text_span_t> &spans, size_t index) {
    for (const auto &span : spans) {
        if (span.begin <= index && index < span.end)
            return &span;
    }
    return nullptr;
}

size_t default_thread_count() {
    const size_t count = std::thread::hardware_concurrency();
    return count == 0 ? 1 : count;
}

size_t scan_block_count(const rect_t &range, bool split_by_y, size_t thread_count) {
    const int span = split_by_y ? range.height() : range.width();
    if (span <= 1 || thread_count <= 1)
        return 1;
    return thread_count < static_cast<size_t>(span) ? thread_count : static_cast<size_t>(span);
}

} // namespace

// 这里的透明图沿用旧规则：四角同色时，把角点色当背景，只匹配剩下的前景点。
int check_transparent(Image *img) {
    if (img->width < 2 || img->height < 2)
        return 0;
    uint c0 = *img->begin();
    bool x = c0 == img->at<uint>(0, img->width - 1) && c0 == img->at<uint>(img->height - 1, 0) &&
             c0 == img->at<uint>(img->height - 1, img->width - 1);
    if (!x)
        return 0;

    int ct = 0;
    for (auto it : *img)
        if (it == c0)
            ++ct;
    int total = img->height * img->width;
    return total * 0.5 <= ct && ct < total ? ct : 0;
}

void get_match_points(const Image &img, vector<uint> &points, int transparent_count) {
    points.clear();
    const int foreground_count = img.width * img.height - transparent_count;
    if (foreground_count > 0)
        points.reserve(static_cast<size_t>(foreground_count));
    uint cbk = *img.begin();
    for (int i = 0; i < img.height; ++i) {
        for (int j = 0; j < img.width; ++j)
            if (cbk != img.at<uint>(i, j))
                points.push_back((i << 16) | j);
    }
}

void build_pic_match_template(Image &img, PicMatchTemplate &match) {
    match.image = &img;
    match.transparent_count = check_transparent(&img);
    match.gray_norm = 0;
    match.transparent_points.clear();
    match.gray.clear();
    if (match.transparent_count) {
        get_match_points(img, match.transparent_points, match.transparent_count);
        return;
    }

    match.gray.fromImage4(img);
    match.gray_norm = sum(match.gray.begin(), match.gray.end());
}

void gen_next(const Image &img, vector<int> &next) {
    next.resize(img.width * img.height);

    auto t = img.ptr<int>(0);
    auto p = next.data();
    p[0] = -1;
    int k = -1, j = 0;
    while (j < static_cast<int>(next.size()) - 1) {
        if (k == -1 || t[k] == t[j]) {
            k++;
            j++;
            p[j] = k;
        } else {
            k = p[k];
        }
    }
}

template <typename VisitFn>
static void flood_connectivity(const ImageBin &bin, ImageBin &visited, int start_x, int start_y, VisitFn visit) {
    // 用栈做连通域遍历，避免大图上递归太深。
    vpoint_t stack;
    stack.push_back(point_t(start_x, start_y));
    visited.at(start_y, start_x) = 1;

    while (!stack.empty()) {
        const point_t p = stack.back();
        stack.pop_back();
        visit(p.x, p.y);

        for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx) {
                if (dx == 0 && dy == 0)
                    continue;

                const int nx = p.x + dx;
                const int ny = p.y + dy;
                if (nx < 0 || ny < 0 || nx >= bin.width || ny >= bin.height)
                    continue;
                if (visited.at(ny, nx) || bin.at(ny, nx) == 0)
                    continue;

                visited.at(ny, nx) = 1;
                stack.push_back(point_t(nx, ny));
            }
        }
    }
}

void Connectivity(const ImageBin &bin, ImageBin &rec) {
    rec.create(bin.width, bin.height);
    std::fill(rec.begin(), rec.end(), 0);
    if (bin.empty())
        return;

    ImageBin visited;
    visited.create(bin.width, bin.height);
    std::fill(visited.begin(), visited.end(), 0);

    int label = 1;
    for (int y = 0; y < bin.height; ++y) {
        for (int x = 0; x < bin.width; ++x) {
            if (bin.at(y, x) == 0 || visited.at(y, x))
                continue;

            const uchar current_label = static_cast<uchar>(label < 255 ? label : 255);
            flood_connectivity(bin, visited, x, y, [&](int px, int py) {
                rec.at(py, px) = current_label;
            });
            if (label < 255)
                ++label;
        }
    }
}

void extractConnectivity(const ImageBin &src, int threshold, std::vector<ImageBin> &out) {
    out.clear();
    if (src.empty())
        return;

    // 先按阈值转成二值图，输出仍保持 0/0xff，方便后续继续按二值图处理。
    ImageBin bin = src;
    for (auto &it : bin)
        it = it > threshold ? 0xffu : 0;

    ImageBin visited;
    visited.create(bin.width, bin.height);
    std::fill(visited.begin(), visited.end(), 0);

    vpoint_t component;
    for (int y = 0; y < bin.height; ++y) {
        for (int x = 0; x < bin.width; ++x) {
            if (bin.at(y, x) == 0 || visited.at(y, x))
                continue;

            component.clear();
            rect_t bounds(x, y, x + 1, y + 1);
            flood_connectivity(bin, visited, x, y, [&](int px, int py) {
                component.push_back(point_t(px, py));
                if (px < bounds.x1)
                    bounds.x1 = px;
                if (py < bounds.y1)
                    bounds.y1 = py;
                if (px + 1 > bounds.x2)
                    bounds.x2 = px + 1;
                if (py + 1 > bounds.y2)
                    bounds.y2 = py + 1;
            });

            ImageBin item;
            item.create(bounds.width(), bounds.height());
            std::fill(item.begin(), item.end(), 0);
            for (const auto &p : component)
                item.at(p.y - bounds.y1, p.x - bounds.x1) = 0xffu;
            out.push_back(item);
        }
    }
}

ImageSearchAlgorithms::ImageSearchAlgorithms() : m_threadPool(default_thread_count()) {
    _x1 = _y1 = 0;
    _dx = _dy = 0;
}

ImageSearchAlgorithms::~ImageSearchAlgorithms() {
}

void ImageSearchAlgorithms::set_offset(int x1, int y1) {
    _x1 = x1;
    _y1 = y1;
}

long ImageSearchAlgorithms::GetPixel(long x, long y, color_t &cr) {
    const int local_x = static_cast<int>(x) - _x1 - _dx;
    const int local_y = static_cast<int>(y) - _y1 - _dy;
    if (!is_valid(local_x, local_y))
        return 0;

    cr = _src.at<color_t>(local_y, local_x);
    return 1;
}

long ImageSearchAlgorithms::CmpColor(color_t color, std::vector<color_df_t> &colors, double sim) {
    for (auto &it : colors) {
        if (color_matches(color, it, sim))
            return 1;
    }

    return 0;
}
static int normalize_dir(int dir) {
    return dir >= 0 && dir <= 8 ? dir : 0;
}

static long long center_distance2(const point_t &p, const rect_t &range) {
    const long long center2x = static_cast<long long>(range.x1) + range.x2 - 1;
    const long long center2y = static_cast<long long>(range.y1) + range.y2 - 1;
    const long long dx = static_cast<long long>(p.x) * 2 - center2x;
    const long long dy = static_cast<long long>(p.y) * 2 - center2y;
    return dx * dx + dy * dy;
}

static long long center_distance2(int x, int y, const rect_t &range) {
    const long long center2x = static_cast<long long>(range.x1) + range.x2 - 1;
    const long long center2y = static_cast<long long>(range.y1) + range.y2 - 1;
    const long long dx = static_cast<long long>(x) * 2 - center2x;
    const long long dy = static_cast<long long>(y) * 2 - center2y;
    return dx * dx + dy * dy;
}

struct CenterRowScan {
    CenterRowScan(int row, int begin_x, int end_x, const rect_t &range)
        : y(row), x1(begin_x), x2(end_x), center2x(static_cast<long long>(range.x1) + range.x2 - 1) {
        left = static_cast<int>(center2x / 2);
        right = left + 1;
        if (left >= x2)
            left = x2 - 1;
        if (right < x1)
            right = x1;
    }

    bool next(int &x) {
        while ((left >= x1 && left < x2) || (right >= x1 && right < x2)) {
            const bool has_left = left >= x1 && left < x2;
            const bool has_right = right >= x1 && right < x2;
            if (has_left && has_right) {
                const long long left_dx = static_cast<long long>(left) * 2 - center2x;
                const long long right_dx = static_cast<long long>(right) * 2 - center2x;
                if (left_dx * left_dx <= right_dx * right_dx) {
                    x = left--;
                    return true;
                }
                x = right++;
                return true;
            }
            if (has_left) {
                x = left--;
                return true;
            }
            x = right++;
            return true;
        }
        return false;
    }

    int y;
    int x1;
    int x2;
    long long center2x;
    int left;
    int right;
};

struct CenterScanNode {
    long long distance = 0;
    int y = 0;
    int x = 0;
    size_t row_index = 0;
};

struct CenterScanNodeGreater {
    bool operator()(const CenterScanNode &lhs, const CenterScanNode &rhs) const {
        if (lhs.distance != rhs.distance)
            return lhs.distance > rhs.distance;
        if (lhs.y != rhs.y)
            return lhs.y > rhs.y;
        return lhs.x > rhs.x;
    }
};

template <typename Fn> static bool for_each_center_scan_point(const rect_t &scan_range, const rect_t &order_range, Fn fn) {
    std::vector<CenterRowScan> rows;
    rows.reserve(static_cast<size_t>(scan_range.height()));
    std::priority_queue<CenterScanNode, std::vector<CenterScanNode>, CenterScanNodeGreater> queue;

    for (int y = scan_range.y1; y < scan_range.y2; ++y) {
        rows.emplace_back(y, scan_range.x1, scan_range.x2, order_range);
        int x = 0;
        if (rows.back().next(x)) {
            queue.push({center_distance2(x, y, order_range), y, x, rows.size() - 1});
        }
    }

    while (!queue.empty()) {
        const CenterScanNode node = queue.top();
        queue.pop();
        if (fn(node.x, node.y))
            return true;

        int x = 0;
        if (rows[node.row_index].next(x))
            queue.push({center_distance2(x, node.y, order_range), node.y, x, node.row_index});
    }

    return false;
}

static bool point_precedes_in_dir(const point_t &lhs, const point_t &rhs, int dir, const rect_t &range) {
    if (rhs.x == -1 || rhs.y == -1)
        return true;

    dir = normalize_dir(dir);
    if (dir == 0)
        return lhs.y < rhs.y || (lhs.y == rhs.y && lhs.x < rhs.x);
    if (dir == 1)
        return lhs.y > rhs.y || (lhs.y == rhs.y && lhs.x < rhs.x);
    if (dir == 2)
        return lhs.y < rhs.y || (lhs.y == rhs.y && lhs.x > rhs.x);
    if (dir == 3)
        return lhs.y > rhs.y || (lhs.y == rhs.y && lhs.x > rhs.x);
    if (dir == 4) {
        const long long lhs_dist = center_distance2(lhs, range);
        const long long rhs_dist = center_distance2(rhs, range);
        if (lhs_dist != rhs_dist)
            return lhs_dist < rhs_dist;
        return lhs.y < rhs.y || (lhs.y == rhs.y && lhs.x < rhs.x);
    }
    if (dir == 5)
        return lhs.x < rhs.x || (lhs.x == rhs.x && lhs.y < rhs.y);
    if (dir == 6)
        return lhs.x > rhs.x || (lhs.x == rhs.x && lhs.y < rhs.y);
    if (dir == 7)
        return lhs.x < rhs.x || (lhs.x == rhs.x && lhs.y > rhs.y);

    return lhs.x > rhs.x || (lhs.x == rhs.x && lhs.y > rhs.y);
}

template <typename Fn> static bool for_each_scan_point(const rect_t &scan_range, int dir, const rect_t &order_range, Fn fn) {
    if (!scan_range.valid())
        return false;

    dir = normalize_dir(dir);
    if (dir == 0) {
        for (int y = scan_range.y1; y < scan_range.y2; ++y)
            for (int x = scan_range.x1; x < scan_range.x2; ++x)
                if (fn(x, y))
                    return true;
    } else if (dir == 1) {
        for (int y = scan_range.y2 - 1; y >= scan_range.y1; --y)
            for (int x = scan_range.x1; x < scan_range.x2; ++x)
                if (fn(x, y))
                    return true;
    } else if (dir == 2) {
        for (int y = scan_range.y1; y < scan_range.y2; ++y)
            for (int x = scan_range.x2 - 1; x >= scan_range.x1; --x)
                if (fn(x, y))
                    return true;
    } else if (dir == 3) {
        for (int y = scan_range.y2 - 1; y >= scan_range.y1; --y)
            for (int x = scan_range.x2 - 1; x >= scan_range.x1; --x)
                if (fn(x, y))
                    return true;
    } else if (dir == 4) {
        // 中心扫描按距离、y、x 顺序直接生成，避免为大区域分配并排序所有坐标。
        return for_each_center_scan_point(scan_range, order_range, fn);
    } else if (dir == 5) {
        for (int x = scan_range.x1; x < scan_range.x2; ++x)
            for (int y = scan_range.y1; y < scan_range.y2; ++y)
                if (fn(x, y))
                    return true;
    } else if (dir == 6) {
        for (int x = scan_range.x2 - 1; x >= scan_range.x1; --x)
            for (int y = scan_range.y1; y < scan_range.y2; ++y)
                if (fn(x, y))
                    return true;
    } else if (dir == 7) {
        for (int x = scan_range.x1; x < scan_range.x2; ++x)
            for (int y = scan_range.y2 - 1; y >= scan_range.y1; --y)
                if (fn(x, y))
                    return true;
    } else {
        for (int x = scan_range.x2 - 1; x >= scan_range.x1; --x)
            for (int y = scan_range.y2 - 1; y >= scan_range.y1; --y)
                if (fn(x, y))
                    return true;
    }

    return false;
}

template <typename Fn> static bool for_each_scan_point(const rect_t &scan_range, int dir, Fn fn) {
    return for_each_scan_point(scan_range, dir, scan_range, fn);
}

static bool point_desc_precedes_in_dir(const point_desc_t &lhs, const point_desc_t &rhs, int dir, const rect_t &range) {
    if (lhs.pos == rhs.pos)
        return lhs.id < rhs.id;

    return point_precedes_in_dir(lhs.pos, rhs.pos, dir, range);
}

static long sort_and_limit_point_desc(vpoint_desc_t &vpd, long dir, size_t max_count, const rect_t &range) {
    std::stable_sort(vpd.begin(), vpd.end(), [dir, range](const point_desc_t &lhs, const point_desc_t &rhs) {
        return point_desc_precedes_in_dir(lhs, rhs, dir, range);
    });
    if (vpd.size() > max_count)
        vpd.resize(max_count);
    return static_cast<long>(vpd.size());
}

long ImageSearchAlgorithms::FindColor(vector<color_df_t> &colors, double sim, int dir, long &x, long &y) {
    const auto prepared_colors = prepared_color_dfs(colors, sim);
    rect_t range(0, 0, _src.width, _src.height);
    // 按扫描方向优先返回最靠前的坐标；同一点再按颜色列表顺序匹配。
    if (for_each_scan_point(range, dir, [&](int j, int i) {
            for (const auto &it : prepared_colors) {
                if (color_matches_prepared(_src.at<color_t>(i, j), it)) {
                    x = j + _x1 + _dx;
                    y = i + _y1 + _dy;
                    return true;
                }
            }
            return false;
        })) {
        return 1;
    }

    x = y = -1;
    return 0;
}

long ImageSearchAlgorithms::FindColorEx(vector<color_df_t> &colors, double sim, int dir, std::wstring &retstr) {
    retstr.clear();
    retstr.reserve(static_cast<size_t>(_max_return_obj_ct + 1) * 16);
    const auto prepared_colors = prepared_color_dfs(colors, sim);
    int find_ct = 0;
    rect_t range(0, 0, _src.width, _src.height);

    for_each_scan_point(range, dir, [&](int j, int i) {
        for (const auto &it : prepared_colors) { // 对每个颜色描述
            if (color_matches_prepared(_src.at<color_t>(i, j), it)) {
                retstr += std::to_wstring(j + _x1 + _dx) + L"," + std::to_wstring(i + _y1 + _dy);
                retstr += L"|";
                ++find_ct;
                return find_ct > _max_return_obj_ct;
            }
        }
        return false;
    });
    if (!retstr.empty() && retstr.back() == L'|')
        retstr.pop_back();
    return find_ct;
}

long ImageSearchAlgorithms::FindColorNum(vector<color_df_t> &colors, double sim) {
    const auto prepared_colors = prepared_color_dfs(colors, sim);
    int find_ct = 0;
    for (int i = 0; i < _src.height; ++i) {
        auto p = _src.ptr<color_t>(i);
        for (int j = 0; j < _src.width; ++j) {
            for (const auto &it : prepared_colors) { // 对每个颜色描述
                if (color_matches_prepared(*p, it)) {
                    ++find_ct;
                    if (find_ct >= INT_MAX)
                        goto _quick_break;
                    break;
                }
            }
            p++;
        }
    }
_quick_break:
    return find_ct;
}

long ImageSearchAlgorithms::FindMultiColor(std::vector<color_df_t> &first_color, std::vector<pt_cr_df_t> &offset_color, double sim,
                               long dir, long &x, long &y) {
    const auto prepared_first_color = prepared_color_dfs(first_color, sim);
    const auto prepared_offset_color = prepared_point_color_dfs(offset_color, sim);
    int max_err_ct = static_cast<int>(offset_color.size() * (1. - sim));
    rect_t range(0, 0, _src.width, _src.height);
    if (for_each_scan_point(range, dir, [&](int j, int i) {
            // step 1. find first color
            for (const auto &it : prepared_first_color) { // 对每个颜色描述
                if (!color_matches_prepared(_src.at<color_t>(i, j), it))
                    continue;

                // 匹配其他坐标
                int err_ct = 0;
                for (const auto &off_cr : prepared_offset_color) {
                    int ptX = j + off_cr.x;
                    int ptY = i + off_cr.y;
                    if (ptX >= 0 && ptX < _src.width && ptY >= 0 && ptY < _src.height) {
                        color_t currentColor = _src.at<color_t>(ptY, ptX);
                        if (!any_color_matches_prepared(currentColor, off_cr.crdfs))
                            ++err_ct;
                    } else {
                        ++err_ct;
                    }
                    if (err_ct > max_err_ct)
                        break;
                }
                if (err_ct <= max_err_ct) {
                    x = j + _x1 + _dx;
                    y = i + _y1 + _dy;
                    return true;
                }
            }
            return false;
        })) {
        return 1;
    }

    x = y = -1;
    return 0;
}

long ImageSearchAlgorithms::FindMultiColorEx(std::vector<color_df_t> &first_color, std::vector<pt_cr_df_t> &offset_color,
                                 double sim, long dir, std::wstring &retstr) {
    retstr.clear();
    retstr.reserve(static_cast<size_t>(_max_return_obj_ct + 1) * 16);
    const auto prepared_first_color = prepared_color_dfs(first_color, sim);
    const auto prepared_offset_color = prepared_point_color_dfs(offset_color, sim);
    int max_err_ct = static_cast<int>(offset_color.size() * (1. - sim));
    int find_ct = 0;
    rect_t range(0, 0, _src.width, _src.height);

    for_each_scan_point(range, dir, [&](int j, int i) {
        // step 1. find first color
        for (const auto &it : prepared_first_color) { // 对每个颜色描述
            if (!color_matches_prepared(_src.at<color_t>(i, j), it))
                continue;

            // 匹配其他坐标
            int err_ct = 0;
            for (const auto &off_cr : prepared_offset_color) {
                int ptX = j + off_cr.x;
                int ptY = i + off_cr.y;
                if (ptX >= 0 && ptX < _src.width && ptY >= 0 && ptY < _src.height) {
                    color_t currentColor = _src.at<color_t>(ptY, ptX);
                    if (!any_color_matches_prepared(currentColor, off_cr.crdfs))
                        ++err_ct;
                } else {
                    ++err_ct;
                }
                if (err_ct > max_err_ct)
                    break;
            }
            if (err_ct <= max_err_ct) {
                retstr += to_wstring(j + _x1 + _dx) + L"," + to_wstring(i + _y1 + _dy);
                retstr += L"|";
                ++find_ct;
                return find_ct > _max_return_obj_ct;
            }
        }
        return false;
    });
    if (!retstr.empty() && retstr.back() == L'|')
        retstr.pop_back();
    return find_ct;
    // x = y = -1;
}

long ImageSearchAlgorithms::FindPic(std::vector<Image *> &pics, color_t dfcolor, double sim, long dir, long &x, long &y) {
    std::vector<PicMatchTemplate> prepared(pics.size());
    std::vector<PicMatchTemplate *> views;
    views.reserve(pics.size());
    for (size_t i = 0; i < pics.size(); ++i) {
        if (!pics[i])
            continue;
        build_pic_match_template(*pics[i], prepared[i]);
        views.push_back(&prepared[i]);
    }
    return FindPic(views, dfcolor, sim, dir, x, y);
}

long ImageSearchAlgorithms::FindPic(std::vector<PicMatchTemplate *> &pics, color_t dfcolor, double sim, long dir, long &x,
                                    long &y) {
    x = y = -1;
    _gray.fromImage4(_src);
    record_sum(_gray);
    // 模板预处理由缓存层完成，这里只做扫描和匹配。
    for (size_t pic_id = 0; pic_id < pics.size(); ++pic_id) {
        auto match = pics[pic_id];
        if (!match || !match->image)
            continue;
        auto pic = match->image;
        const int use_ts_match = match->transparent_count;

        rect_t matchRect(0, 0, _src.width, _src.height);
        matchRect.shrinkRect(pic->width, pic->height);
        if (!matchRect.valid())
            continue;
        const int max_err_ct = static_cast<int>((pic->height * pic->width - use_ts_match) * (1.0 - sim));
        if (for_each_scan_point(matchRect, dir, [&](int j, int i) {
                int match_ret = (use_ts_match ? trans_match<false>(j, i, pic, dfcolor, match->transparent_points, max_err_ct)
                                              : real_match(j, i, &match->gray, match->gray_norm, sim));
                if (match_ret) {
                    x = j + _x1 + _dx;
                    y = i + _y1 + _dy;
                    return true;
                }
                return false;
            })) {
            return static_cast<long>(pic_id);
        }
    }         // end for pics
    return -1;
}

long ImageSearchAlgorithms::FindPicTh(std::vector<Image *> &pics, color_t dfcolor, double sim, long dir, long &x, long &y) {
    std::vector<PicMatchTemplate> prepared(pics.size());
    std::vector<PicMatchTemplate *> views;
    views.reserve(pics.size());
    for (size_t i = 0; i < pics.size(); ++i) {
        if (!pics[i])
            continue;
        build_pic_match_template(*pics[i], prepared[i]);
        views.push_back(&prepared[i]);
    }
    return FindPicTh(views, dfcolor, sim, dir, x, y);
}

long ImageSearchAlgorithms::FindPicTh(std::vector<PicMatchTemplate *> &pics, color_t dfcolor, double sim, long dir,
                                      long &x, long &y) {
    x = y = -1;
    _gray.fromImage4(_src);
    record_sum(_gray);
    std::vector<rect_t> blocks;
    // 将小循环放在最外面，提高处理速度
    for (size_t pic_id = 0; pic_id < pics.size(); ++pic_id) {
        auto match = pics[pic_id];
        if (!match || !match->image)
            continue;
        auto pic = match->image;
        const int use_ts_match = match->transparent_count;
        auto pgimg = &match->gray;
        rect_t matchRect(0, 0, _src.width, _src.height);
        matchRect.shrinkRect(pic->width, pic->height);
        if (!matchRect.valid())
            continue;
        const int max_err_ct = static_cast<int>((pic->height * pic->width - use_ts_match) * (1.0 - sim));
        const bool split_by_y = matchRect.width() > matchRect.height();
        const size_t block_count = scan_block_count(matchRect, split_by_y, m_threadPool.getThreadNum());
        matchRect.divideBlock(static_cast<int>(block_count), split_by_y, blocks);
        std::vector<std::future<point_t>> results;
        results.reserve(blocks.size());
        for (size_t i = 0; i < blocks.size(); ++i) {
            results.push_back(m_threadPool.enqueue(
                [this, dfcolor, dir, match, pgimg, matchRect, max_err_ct](rect_t block, Image *pic, int use_ts_match,
                                                                          double sim) {
                    point_t found(-1, -1);
                    for_each_scan_point(block, dir, matchRect, [&](int j, int i) {
                        int match_ret =
                            (use_ts_match ? trans_match<false>(j, i, pic, dfcolor, match->transparent_points, max_err_ct)
                                          : real_match(j, i, pgimg, match->gray_norm, sim));
                        if (match_ret) {
                            found = point_t(j + _x1 + _dx, i + _y1 + _dy);
                            return true;
                        }
                        return false;
                    });
                    if (found.x != -1)
                        return found;
                    return point_t(-1, -1);
                },
                blocks[i], pic, use_ts_match, sim));
            // results.push_back(r);
        }
        // wait all
        point_t best(-1, -1);
        rect_t globalMatchRect(matchRect.x1 + _x1 + _dx, matchRect.y1 + _y1 + _dy, matchRect.x2 + _x1 + _dx,
                               matchRect.y2 + _y1 + _dy);
        for (auto &&f : results) {
            point_t p = f.get();
            if (p.x != -1 && point_precedes_in_dir(p, best, dir, globalMatchRect))
                best = p;
        }
        if (best.x != -1) {
            x = best.x;
            y = best.y;
            return static_cast<long>(pic_id);
        }

    } // end for pics
    return -1;
}

long ImageSearchAlgorithms::FindPicEx(std::vector<Image *> &pics, color_t dfcolor, double sim, long dir, vpoint_desc_t &vpd) {
    std::vector<PicMatchTemplate> prepared(pics.size());
    std::vector<PicMatchTemplate *> views;
    views.reserve(pics.size());
    for (size_t i = 0; i < pics.size(); ++i) {
        if (!pics[i])
            continue;
        build_pic_match_template(*pics[i], prepared[i]);
        views.push_back(&prepared[i]);
    }
    return FindPicEx(views, dfcolor, sim, dir, vpd);
}

long ImageSearchAlgorithms::FindPicEx(std::vector<PicMatchTemplate *> &pics, color_t dfcolor, double sim, long dir,
                                      vpoint_desc_t &vpd) {
    vpd.clear();
    int match_ret = 0;
    _gray.fromImage4(_src);
    record_sum(_gray);
    for (size_t pic_id = 0; pic_id < pics.size(); ++pic_id) {
        auto match = pics[pic_id];
        if (!match || !match->image)
            continue;
        auto pic = match->image;
        const int use_ts_match = match->transparent_count;

        rect_t matchRect(0, 0, _src.width, _src.height);
        matchRect.shrinkRect(pic->width, pic->height);
        if (!matchRect.valid())
            continue;
        const int max_err_ct = static_cast<int>((pic->height * pic->width - use_ts_match) * (1.0 - sim));
        for (int i = matchRect.y1; i < matchRect.y2; ++i) {
            for (int j = matchRect.x1; j < matchRect.x2; ++j) {
                match_ret = (use_ts_match ? trans_match<false>(j, i, pic, dfcolor, match->transparent_points, max_err_ct)
                                          : real_match(j, i, &match->gray, match->gray_norm, sim));
                if (match_ret) {
                    point_desc_t pd = {static_cast<int>(pic_id), point_t(j + _x1 + _dx, i + _y1 + _dy)};

                    vpd.push_back(pd);
                }

            } // end for j
        }     // end for i
    }         // end for pics

    rect_t resultRange(_x1 + _dx, _y1 + _dy, _x1 + _dx + _src.width, _y1 + _dy + _src.height);
    return sort_and_limit_point_desc(vpd, dir, _max_return_obj_ct, resultRange);
}

long ImageSearchAlgorithms::FindPicExTh(std::vector<Image *> &pics, color_t dfcolor, double sim, long dir, vpoint_desc_t &vpd) {
    std::vector<PicMatchTemplate> prepared(pics.size());
    std::vector<PicMatchTemplate *> views;
    views.reserve(pics.size());
    for (size_t i = 0; i < pics.size(); ++i) {
        if (!pics[i])
            continue;
        build_pic_match_template(*pics[i], prepared[i]);
        views.push_back(&prepared[i]);
    }
    return FindPicExTh(views, dfcolor, sim, dir, vpd);
}

long ImageSearchAlgorithms::FindPicExTh(std::vector<PicMatchTemplate *> &pics, color_t dfcolor, double sim, long dir,
                                        vpoint_desc_t &vpd) {
    vpd.clear();
    _gray.fromImage4(_src);
    record_sum(_gray);
    std::vector<rect_t> blocks;
    // 将小循环放在最外面，提高处理速度
    for (size_t pic_id = 0; pic_id < pics.size(); ++pic_id) {
        auto match = pics[pic_id];
        if (!match || !match->image)
            continue;
        auto pic = match->image;
        const int use_ts_match = match->transparent_count;
        auto pgimg = &match->gray;
        rect_t matchRect(0, 0, _src.width, _src.height);
        matchRect.shrinkRect(pic->width, pic->height);
        if (!matchRect.valid())
            continue;
        const int max_err_ct = static_cast<int>((pic->height * pic->width - use_ts_match) * (1.0 - sim));
        const bool split_by_y = matchRect.width() > matchRect.height();
        const size_t block_count = scan_block_count(matchRect, split_by_y, m_threadPool.getThreadNum());
        matchRect.divideBlock(static_cast<int>(block_count), split_by_y, blocks);
        std::vector<std::future<vpoint_t>> results;
        results.reserve(blocks.size());
        for (size_t i = 0; i < blocks.size(); ++i) {
            results.push_back(m_threadPool.enqueue(
                [this, dfcolor, match, pgimg, max_err_ct](rect_t block, Image *pic, int use_ts_match,
                                                          double sim) -> vpoint_t {
                    vpoint_t vp;
                    for (int i = block.y1; i < block.y2; ++i) {
                        for (int j = block.x1; j < block.x2; ++j) {
                            int match_ret =
                                (use_ts_match
                                     ? trans_match<false>(j, i, pic, dfcolor, match->transparent_points, max_err_ct)
                                     : real_match(j, i, pgimg, match->gray_norm, sim));
                            if (match_ret) {
                                vp.push_back(point_t(j + _x1 + _dx, i + _y1 + _dy));
                            }

                        } // end for j
                    }     // end for i
                    return vp;
                },
                blocks[i], pic, use_ts_match, sim));
            // results.push_back(r);
        }
        // wait all
        for (auto &&f : results) {
            vpoint_t vp = f.get();
            if (vp.size() > 0) {
                for (auto &p : vp) {
                    point_desc_t pd = {static_cast<int>(pic_id), p};
                    vpd.push_back(pd);
                }

                // return pic_id;
            }
        }
    }

    rect_t resultRange(_x1 + _dx, _y1 + _dy, _x1 + _dx + _src.width, _y1 + _dy + _src.height);
    return sort_and_limit_point_desc(vpd, dir, _max_return_obj_ct, resultRange);
}

long ImageSearchAlgorithms::FindColorBlock(long count, long height, long width, long &x, long &y) {
    x = y = -1;
    if (_binary.empty() || count <= 0 || height <= 0 || width <= 0 || height > _binary.height || width > _binary.width)
        return 0;

    record_sum(_binary);
    for (int i = 0; i <= _binary.height - height; ++i) {
        for (int j = 0; j <= _binary.width - width; ++j) {
            if (region_sum(j, i, j + width, i + height) >= count) {
                x = j + _x1 + _dx;
                y = i + _y1 + _dy;
                return 1;
            }
        }
    }
    return 0;
}

long ImageSearchAlgorithms::FindColorBlockEx(long count, long height, long width, std::wstring &retstr) {
    retstr.clear();
    if (_binary.empty() || count <= 0 || height <= 0 || width <= 0 || height > _binary.height || width > _binary.width)
        return 0;

    record_sum(_binary);
    int cnt = 0;
    for (int i = 0; i <= _binary.height - height; ++i) {
        for (int j = 0; j <= _binary.width - width; ++j) {
            if (region_sum(j, i, j + width, i + height) >= count) {
                retstr += to_wstring(j + _x1 + _dx) + L"," + to_wstring(i + _y1 + _dy) + L"|";
                ++cnt;
                if (cnt > _max_return_obj_ct)
                    goto _quick_return;
            }
        }
    }
_quick_return:
    if (cnt) {
        retstr.pop_back();
    }
    return cnt;
}

long ImageSearchAlgorithms::Ocr(Dictionary &dict, double sim, wstring &retstr) {
    retstr.clear();
    std::map<point_t, ocr_rec_t> ps;
    bin_ocr(dict, sim, ps);
    for (auto &it : ps) {
        retstr += it.second.text;
    }
    return 1;
}

long ImageSearchAlgorithms::OcrEx(Dictionary &dict, double sim, std::wstring &retstr) {
    retstr.clear();
    std::map<point_t, ocr_rec_t> ps;
    bin_ocr(dict, sim, ps);
    // x1,y1,str....|x2,y2,str2...|...
    int find_ct = 0;
    for (auto &it : ps) {
        retstr += std::to_wstring(it.first.x + _x1 + _dx);
        retstr += L",";
        retstr += std::to_wstring(it.first.y + _y1 + _dy);
        retstr += L",";
        retstr += it.second.text;
        retstr += L"|";
        ++find_ct;
        if (find_ct > _max_return_obj_ct)
            break;
    }
    if (!retstr.empty() && retstr.back() == L'|')
        retstr.pop_back();
    return find_ct;
}

long ImageSearchAlgorithms::FindStr(std::map<point_t, ocr_rec_t> &ps, const vector<wstring> &vstr, long &retx,
                        long &rety) {
    retx = rety = -1;

    std::vector<ocr_text_span_t> spans;
    const std::wstring str = build_ocr_text_spans(ps, spans);

    for (size_t i = 0; i < vstr.size(); ++i) {
        if (vstr[i].empty())
            continue;

        const size_t idx = str.find(vstr[i]);
        if (idx == std::wstring::npos)
            continue;

        const ocr_text_span_t *span = find_ocr_text_span(spans, idx);
        if (span == nullptr)
            continue;

        retx = span->point.x + _x1 + _dx;
        rety = span->point.y + _y1 + _dy;
        return static_cast<long>(i);
    }

    return -1;
}

long ImageSearchAlgorithms::FindStrEx(std::map<point_t, ocr_rec_t> &ps, const vector<wstring> &vstr,
                          std::wstring &retstr) {
    // 描述：查找屏幕指定位置的字符（或者字符串）位置，返回所有出现的坐标（注意与FindStr接口的区别）！！！
    //----------------------步骤-----------------
    //  step 1. 获取指定位置的字符及坐标信息
    //  step 2. 拼接字符，形成完整字符串 str
    //  step 2.对每个目标字符 ti , 查找其在str中的位置，并记录 index(如果存在）
    //  step 4.根据index ,获取其坐标，并将坐标转化为字符串，拼接到返回值
    //  step 5. 回到第3步

    retstr.clear();

    std::vector<ocr_text_span_t> spans;
    const std::wstring str = build_ocr_text_spans(ps, spans);

    int find_ct = 0;
    for (size_t i = 0; i < vstr.size(); ++i) {
        if (vstr[i].empty())
            continue;

        size_t search_from = 0;
        do {
            const size_t index = str.find(vstr[i], search_from);
            if (index == std::wstring::npos)
                break;

            const ocr_text_span_t *span = find_ocr_text_span(spans, index);
            if (span != nullptr) {
                retstr += std::to_wstring(i);
                retstr += L",";
                retstr += std::to_wstring(span->point.x + _x1 + _dx);
                retstr += L",";
                retstr += std::to_wstring(span->point.y + _y1 + _dy);
                retstr += L"|";
                ++find_ct;
                if (find_ct > _max_return_obj_ct)
                    goto _quick_return;
            }

            search_from = index + 1;
        } while (1);
    }
_quick_return:
    if (!retstr.empty() && retstr.back() == L'|')
        retstr.pop_back();
    return find_ct;
}

long ImageSearchAlgorithms::FindLine(std::wstring &outStr) {
    outStr.clear();
    int h = static_cast<int>(sqrt(_binary.width * _binary.width + _binary.height * _binary.height)) + 2;
    _sum.create(360, h);
    // 行：距离，列：角度
    _sum.fill(0);
    std::array<double, 360> cos_table;
    std::array<double, 360> sin_table;
    for (int t = 0; t < 360; ++t) {
        const double radians = t * 0.0174532925;
        cos_table[t] = cos(radians);
        sin_table[t] = sin(radians);
    }
    for (int i = 0; i < _binary.height; i++) {
        for (int j = 0; j < _binary.width; j++) {
            if (_binary.at(i, j) == WORD_COLOR) {
                for (int t = 0; t < 360; t++) {
                    int d = static_cast<int>(j * cos_table[t] + i * sin_table[t]);
                    assert(d <= h);
                    if (d >= 0)
                        _sum.at<int>(d, t)++;
                }
            }
        }
    }
    int maxRow = 0, maxCol = 0;
    int maxval = -1;
    for (int i = 0; i < _sum.height; i++) {
        for (int j = 0; j < _sum.width; j++) {
            if (_sum.at<int>(i, j) > maxval) {
                maxRow = i;
                maxCol = j;
                maxval = _sum.at<int>(i, j);
            }
        }
    }
    // setlog("degree=%d,dis=%d,val=%d", maxCol, maxRow, maxval);
    outStr = to_wstring(maxCol) + L"," + to_wstring(maxRow);
    return maxval;
}

template <bool nodfcolor>
long ImageSearchAlgorithms::trans_match(long x, long y, Image *timg, color_t dfcolor, const vector<uint> &pts, int max_error) {
    int err_ct = 0;
    int left = 0;
    int right = static_cast<int>(pts.size()) - 1;
    while (left <= right) {
        if (nodfcolor) {
            if (_src.at<uint>(y + PTY(pts[left]), x + PTX(pts[left])) != timg->at<uint>(PTY(pts[left]), PTX(pts[left])))
                ++err_ct;
            if (left != right) {
                if (_src.at<uint>(y + PTY(pts[right]), x + PTX(pts[right])) !=
                    timg->at<uint>(PTY(pts[right]), PTX(pts[right])))
                    ++err_ct;
            }
        } else {
            color_t cr1, cr2;
            cr1 = _src.at<color_t>(y + PTY(pts[left]), x + PTX(pts[left]));
            cr2 = timg->at<color_t>(PTY(pts[left]), PTX(pts[left]));
            if (!IN_RANGE(cr1, cr2, dfcolor))
                ++err_ct;
            if (left != right) {
                cr1 = _src.at<color_t>(y + PTY(pts[right]), x + PTX(pts[right]));
                cr2 = timg->at<color_t>(PTY(pts[right]), PTX(pts[right]));
                if (!IN_RANGE(cr1, cr2, dfcolor))
                    ++err_ct;
            }
        }

        ++left;
        --right;
        if (err_ct > max_error)
            return 0;
    }
    return 1;
}

long ImageSearchAlgorithms::real_match(long x, long y, ImageBin *timg, int tnorm, double sim) {
    // quick check
    if ((double)abs(tnorm - region_sum(x, y, x + timg->width, y + timg->height)) > (double)tnorm * (1.0 - sim))
        return 0;
    int err = 0;
    int maxErr = static_cast<int>((1.0 - sim) * tnorm);
    for (int i = 0; i < timg->height; i++) {
        auto ptr = _gray.ptr(y + i) + x;
        auto ptr2 = timg->ptr(i);
        for (int j = 0; j < timg->width; j++) {
            err += abs(*ptr - *ptr2);
            ptr++;
            ptr2++;
        }
        if (err > maxErr)
            return 0;
    }

    return 1;
}

void ImageSearchAlgorithms::record_sum(const ImageBin &gray) {
    // 为了减少边界判断，这里多多加一行一列
    _sum.create(gray.width + 1, gray.height + 1);
    _sum.fill(0);
    int m = _sum.height;
    int n = _sum.width;
    for (int i = 1; i < m; i++) {
        for (int j = 1; j < n; j++) {
            int s = 0;

            s += _sum.at<int>(i - 1, j);

            s += _sum.at<int>(i, j - 1);

            s -= _sum.at<int>(i - 1, j - 1);
            s += (int)gray.at(i - 1, j - 1);
            _sum.at<int>(i, j) = s;
        }
    }
}

int ImageSearchAlgorithms::region_sum(int x1, int y1, int x2, int y2) {
    int ans = _sum.at<int>(y2, x2) - _sum.at<int>(y2, x1) - _sum.at<int>(y1, x2) + _sum.at<int>(y1, x1);
    return ans;
}

constexpr int MIN_CUT_W = 5;
constexpr int MIN_CUT_H = 2;

int ImageSearchAlgorithms::get_bk_color(inputbin bin) {
    int y[256] = {0};
    auto ptr = bin.pixels.data();
    int n = static_cast<int>(bin.pixels.size());
    for (int i = 0; i < n; ++i)
        y[ptr[i]]++;
    // scan max
    int m = 0;
    for (int i = 1; i < 256; ++i) {
        if (y[i] > y[m])
            m = i;
    }
    return m;
}

void ImageSearchAlgorithms::bgr2binary(vector<color_df_t> &colors) {
    _binary.clear();
    if (_src.empty())
        return;
    int ncols = _src.width, nrows = _src.height;
    _binary.create(_src.width, _src.height);
    for (int i = 0; i < nrows; ++i) {
        auto psrc = _src.ptr<color_t>(i);

        auto pbin = _binary.ptr(i);
        for (int j = 0; j < ncols; ++j) {
            *pbin = WORD_BKCOLOR;
            for (auto &it : colors) { // 对每个颜色描述
                if (IN_RANGE(*psrc, it.color, it.df)) {
                    *pbin = WORD_COLOR;
                    break;
                }
            }
            ++pbin;
            ++psrc;
        }
    }
    // test
    // cv::imwrite("src.png", _src);
    // cv::imwrite("binary.png", _binary);
}

// 二值化
void ImageSearchAlgorithms::bgr2binarybk(const vector<color_df_t> &bk_colors) {
    _binary.clear();
    if (_src.empty())
        return;

    // 创建二值图
    _binary.create(_src.width, _src.height);
    memset(_binary.pixels.data(), WORD_BKCOLOR, _binary.size());
    int n = _binary.size();
    auto pdst = _binary.data();
    if (bk_colors.size() == 0) { // auto
        // 转为灰度图
        _gray.fromImage4(_src);

        // 获取背景颜色
        int bkcolor = get_bk_color(_gray);

        auto pgray = _gray.data();
        for (int i = 0; i < n; ++i) {
            pdst[i] = (std::abs((int)pgray[i] - bkcolor) < 20 ? WORD_BKCOLOR : WORD_COLOR);
        }
    } else {
        for (int i = 0; i < n; ++i) {
            auto c = (color_t *)(_src.pdata + i * 4);
            bool is_bk_color = false;
            for (const auto &bk : bk_colors) {
                if (IN_RANGE(*c, bk.color, bk.df)) {
                    is_bk_color = true;
                    break;
                }
            }
            if (!is_bk_color) {
                pdst[i] = WORD_COLOR;
            }
        }
    }
}

// 垂直方向投影到x轴
void ImageSearchAlgorithms::binshadowx(const rect_t &rc, std::vector<rect_t> &out_put) {
    // qDebug("in x rc:%d,%d,%d,%d", rc.x1, rc.y1, rc.x2, rc.y2);
    out_put.clear();
    // ys.clear();
    // Mat paintx(binary.size(), CV_8UC1, cv::Scalar(255));
    // //创建一个全白图片，用作显示

    // int* blackcout = new int[binary.cols];
    std::vector<int> vx;
    vx.resize(_binary.width);
    memset(&vx[0], 0, _binary.width * 4);
    for (int j = rc.x1; j < rc.x2; j++) {
        for (int i = rc.y1; i < rc.y2; i++) {
            if (_binary.at(i, j) == WORD_COLOR) {
                vx[j]++; // 垂直投影按列在x轴进行投影
            }
        }
    }

    int startindex = 0;
    int endindex = 0;
    bool inblock = false; // 是否遍历到字符位置
    rect_t roi;
    for (int j = rc.x1; j < rc.x2; j++) {
        if (!inblock && vx[j] != 0) // 进入有字符区域
        {
            inblock = true;
            startindex = j;
            // std::cout << "startindex:" << startindex << std::endl;
        }
        // if (inblock&&vx[j] == 0) //进入空白区
        else if (inblock && vx[j] == 0 && j - startindex >= MIN_CUT_W) // 进入空白区域，且宽度不小于1
        {
            endindex = j;
            inblock = false;
            // Mat roi = binary.colRange(startindex, endindex + 1);

            roi.x1 = startindex;
            roi.y1 = rc.y1;
            roi.x2 = endindex;
            roi.y2 = rc.y2;
            // qDebug("out xrc:%d,%d,%d,%d", roi.x1, roi.y1, roi.x2, roi.y2);
            out_put.push_back(roi);
        }
    }
    // special case
    if (inblock) {
        roi.x1 = startindex;
        roi.y1 = rc.y1;
        roi.x2 = rc.x2;
        roi.y2 = rc.y2;
        out_put.push_back(roi);
    }
}
// 投影到y轴
void ImageSearchAlgorithms::binshadowy(const rect_t &rc, std::vector<rect_t> &out_put) {
    // qDebug("in y rc:%d,%d,%d,%d", rc.x1, rc.y1, rc.x2, rc.y2);
    out_put.clear();
    // 是否为白色或者黑色根据二值图像的处理得来
    //  Mat painty(binary.size(), CV_8UC1, cv::Scalar(255)); //初始化为全白
    // 水平投影
    //  int* pointcount = new int[binary.rows]; //在二值图片中记录行中特征点的个数
    std::vector<int> vy;
    vy.resize(_binary.height);
    memset(&vy[0], 0, _binary.height * 4); // 注意这里需要进行初始化

    for (int i = rc.y1; i < rc.y2; i++) {
        for (int j = rc.x1; j < rc.x2; j++) {
            if (_binary.at(i, j) == WORD_COLOR) {
                vy[i]++; // 记录每行中黑色点的个数 //水平投影按行在y轴上的投影
            }
        }
    }

    // std::vector<Mat> result;
    int startindex = 0;
    int endindex = 0;
    bool inblock = false; // 是否遍历到字符位置
    rect_t roi;
    for (int i = rc.y1; i < rc.y2; i++) {
        if (!inblock && vy[i] != 0) // 进入有字符区域
        {
            inblock = true;
            startindex = i;
            // std::cout << "startindex:" << startindex << std::endl;
        }
        // if (inblock&&vy[i] == 0) //进入空白区
        if (inblock && vy[i] == 0 && i - startindex >= MIN_CUT_H) // 进入空白区,且高度不小于1
        {
            endindex = i;
            inblock = false;

            roi.x1 = rc.x1;
            roi.y1 = startindex;
            roi.x2 = rc.x2;
            roi.y2 = endindex;
            out_put.push_back(roi);
        }
    }

    if (inblock) {
        roi.x1 = rc.x1;
        roi.y1 = startindex;
        roi.x2 = rc.x2;
        roi.y2 = rc.y2;
        out_put.push_back(roi);
    }
}

bool ImageSearchAlgorithms::bin_image_cut(int min_word_h, const rect_t &inrc, rect_t &outrc) {
    rect_t rc(max(0, inrc.x1), max(0, inrc.y1), min(_binary.width, inrc.x2), min(_binary.height, inrc.y2));
    outrc = rc;
    if (_binary.empty() || !rc.valid()) {
        outrc = rect_t();
        return false;
    }

    auto row_has_word = [&](int y) {
        for (int x = rc.x1; x < rc.x2; ++x) {
            if (_binary.at(y, x) == WORD_COLOR)
                return true;
        }
        return false;
    };

    int top = rc.y1;
    while (top < rc.y2 && !row_has_word(top))
        ++top;
    if (top == rc.y2) {
        outrc = rect_t(rc.x1, rc.y1, rc.x1, rc.y1);
        return false;
    }

    int bottom = rc.y2 - 1;
    while (bottom >= top && !row_has_word(bottom))
        --bottom;
    outrc.y1 = top;
    if (bottom + 1 - top > min_word_h)
        outrc.y2 = bottom + 1;

    auto col_has_word = [&](int x) {
        for (int y = rc.y1; y < rc.y2; ++y) {
            if (_binary.at(y, x) == WORD_COLOR)
                return true;
        }
        return false;
    };

    int left = rc.x1;
    while (left < rc.x2 && !col_has_word(left))
        ++left;
    if (left == rc.x2) {
        outrc = rect_t(rc.x1, rc.y1, rc.x1, rc.y1);
        return false;
    }

    int right = rc.x2 - 1;
    while (right >= left && !col_has_word(right))
        --right;
    outrc.x1 = left;
    outrc.x2 = right + 1;
    return outrc.valid();
}

void ImageSearchAlgorithms::get_rois(int min_word_h, std::vector<rect_t> &vroi) {
    vroi.clear();
    if (_binary.empty())
        return;

    std::vector<rect_t> vrcx, vrcy;
    rect_t rc;
    rc.x1 = rc.y1 = 0;
    rc.x2 = _binary.width;
    rc.y2 = _binary.height;
    binshadowy(rc, vrcy);
    for (size_t i = 0; i < vrcy.size(); ++i) {
        binshadowx(vrcy[i], vrcx);
        for (size_t j = 0; j < vrcx.size(); ++j) {
            if (vrcx[j].width() >= min_word_h && !bin_image_cut(min_word_h, vrcx[j], vrcx[j]))
                continue;
            vroi.push_back(vrcx[j]);
        }
    }
}

void ImageSearchAlgorithms::get_rois(int min_word_w, int min_word_h, int padding, std::vector<rect_t> &vroi) {
    vroi.clear();
    if (_binary.empty())
        return;

    min_word_w = max(1, min_word_w);
    min_word_h = max(1, min_word_h);
    padding = max(0, padding);

    std::vector<rect_t> vrcx, vrcy;
    rect_t rc(0, 0, _binary.width, _binary.height);
    binshadowy(rc, vrcy);
    for (const auto &line_rc : vrcy) {
        binshadowx(line_rc, vrcx);
        for (auto word_rc : vrcx) {
            rect_t cut_rc;
            if (!bin_image_cut(min_word_h, word_rc, cut_rc))
                continue;
            if (cut_rc.width() < min_word_w || cut_rc.height() < min_word_h)
                continue;

            cut_rc.x1 = max(0, cut_rc.x1 - padding);
            cut_rc.y1 = max(0, cut_rc.y1 - padding);
            cut_rc.x2 = min(_binary.width, cut_rc.x2 + padding);
            cut_rc.y2 = min(_binary.height, cut_rc.y2 + padding);
            vroi.push_back(cut_rc);
        }
    }
}

inline int full_match(const ImageBin &binary, rect_t &rc, const uint8_t *data) {
    // 匹配
    int idx = 0;
    for (int x = rc.x1; x < rc.x2; ++x) {
        for (int y = rc.y1; y < rc.y2; ++y) {
            int val = GET_BIT(data[idx / 8], idx & 7);
            if (binary.at(y, x) != val)
                return 0;
            idx++;
        }
    }
    return 1;
}

inline int part_match(const ImageBin &binary, rect_t &rc, int max_error, const uint8_t *data) {
    // 匹配
    int err_ct = 0;
    int idx = 0;
    for (int x = rc.x1; x < rc.x2; ++x) {
        for (int y = rc.y1; y < rc.y2; ++y) {
            int val = GET_BIT(data[idx / 8], idx & 7);
            if (binary.at(y, x) != val) {
                ++err_ct;
                if (err_ct > max_error)
                    return err_ct;
            }
            idx++;
        }
    }
    return err_ct;
}

inline void fill_rect(ImageBin &record, const rect_t &rc) {
    int w = rc.width();
    for (int y = rc.y1; y < rc.y2; ++y) {
        uchar *p = record.ptr(y) + rc.x1;
        memset(p, 1, sizeof(uchar) * w);
    }
}

int binarySearch(const word1_t a[], int bidx, int eidx, int target) // 循环实现
{
    int low = bidx, high = eidx, middle;
    while (low < high) {
        middle = (low + high) / 2;
        if (target == a[middle].info.bit_cnt)
            return middle;
        else if (target > a[middle].info.bit_cnt)
            low = middle + 1;
        else if (target < a[middle].info.bit_cnt)
            high = middle;
    }
    return -1;
};

// 完全匹配 待识别文字不能含有任何噪声
/*
算法
f(p,size) 为从点p开始，大小为size的矩形块像素之和,这个函数使用查表法快速计算
字库:D 字的像素范围:m-M 字的大小范围:size_min-size_max
识别图像:src
for each point in src:
        if f(p,size_max)<m //像素太少
                continue;
        if f(p,size_min)>M //像素太多
                continue;
        //像素合适
        for each w in D
                if f(p,w_size)==w_cnt
                        ok=match(...) //作最后的匹配
                        if ok
                                add w to result;
                                delete rect(p,w_size);
                        else
                                continue;//to next w
                        endif
                end
        end
end
*/
void ImageSearchAlgorithms::_bin_ocr(const Dictionary &dict, std::map<point_t, ocr_rec_t> &ps) {
    int px, py;
    if (_binary.empty())
        return;
    //
    record_sum(_binary);
    // find cnt range
    // find width and height range;
    int cnt_min = 255 * 255, cnt_max = 0;
    int w_min = 255, h_min = 255;
    int w_max = 0, h_max = 0;
    for (auto &it : dict.words) {
        cnt_min = min(cnt_min, it.info.bit_cnt);
        cnt_max = max(cnt_max, it.info.bit_cnt);
        w_min = min(w_min, it.info.w);
        h_min = min(h_min, it.info.h);
        w_max = max(w_max, it.info.w);
        h_max = max(h_max, it.info.h);
    }

    // 将所有字库按照大小分成几类，对于每个大小根据像素密度查找对应的符合字库
    auto makeinfo = [](int begin, int end, int szh, int szw) {
        return std::make_pair(begin << 16 | end, szh << 8 | szw);
    };
    vector<std::pair<int, int>> dict_sz;
    auto &vword = dict.words;
    // 32 begin(8)
    dict_sz.push_back(makeinfo(0, 0, dict.words[0].info.h, dict.words[0].info.w));
    for (size_t i = 1; i < vword.size(); ++i) {
        int sz = vword[i].info.h << 8 | vword[i].info.w;
        if (dict_sz.back().second != sz) {
            dict_sz.back().first |= static_cast<int>(i);                                     // fix old end
            dict_sz.push_back(std::make_pair(static_cast<int>(i) << 16, sz)); // add new begin
        }
    }
    dict_sz.back().first |= static_cast<int>(vword.size());

    // 遍历行
    for (py = 0; py < _binary.height - h_min + 1; ++py) {
        // 遍历列
        for (px = 0; px < _binary.width - w_min + 1; ++px) {
            if (_record.at(py, px))
                continue;
            // 检测像素密度
            if (region_sum(px, py, min(px + w_max, _binary.width), min(py + h_max, _binary.height)) <
                cnt_min) // too less
                continue;
            if (region_sum(px, py, px + w_min, py + h_min) > cnt_max) // too much
                continue;
            point_t pt;
            pt.x = px;
            pt.y = py;
            int k = 0;
            for (size_t k = 0; k < dict_sz.size(); ++k) {
                int h = dict_sz[k].second >> 8, w = dict_sz[k].second & 0xff;
                rect_t crc;
                crc.x1 = px;
                crc.y1 = py;
                crc.x2 = px + w;
                crc.y2 = py + h;
                // 边界检查
                if (crc.y2 > _binary.height || crc.x2 > _binary.width)
                    continue;

                int fidx = dict_sz[k].first >> 16, eidx = dict_sz[k].first & 0xffff;

                // quick check
                int cnt_src = region_sum(crc.x1, crc.y1, crc.x2, crc.y2);
                if (cnt_src < vword[fidx].info.bit_cnt || cnt_src > vword[eidx - 1].info.bit_cnt)
                    continue;
                int tidx = binarySearch(&vword[0], fidx, eidx, cnt_src);
                if (tidx == -1)
                    continue;
                int tleft = tidx, tright = tidx;
                while (tleft > 0 && vword[tleft - 1].info.bit_cnt == cnt_src)
                    --tleft;
                while (tright < eidx && vword[tright].info.bit_cnt == cnt_src)
                    ++tright;
                int matched = 0;
                // match
                tidx = tleft;
                while (tidx < tright) {
                    auto &it = vword[tidx++];
                    matched = full_match(_binary, crc, it.data.data());
                    if (matched) {
                        // final check
                        // check right col is empty/background
                        int rs = 0;
                        if (crc.x2 < _binary.width) {
                            for (int k = crc.y1; k < crc.y2; k++)
                                rs += _binary.at(k, crc.x2);
                        }
                        if (rs < it.info.h / 2) {
                            ocr_rec_t ocr_res;
                            ocr_res.left_top = pt;
                            ocr_res.right_bottom = point_t(crc.x2, crc.y2);
                            ocr_res.text = it.info.name;
                            ocr_res.confidence = 1.0;
                            ps[pt] = ocr_res;
                            fill_rect(_record, crc);
                            // break;//words
                            break;
                        } else {
                            // not matched
                            matched = 0;
                        }
                    }
                }
                if (matched)
                    break;
            } // end for words
        }     // end for j
    }         // end for i
}
// 模糊匹配 待识别区域可以含有噪声
void ImageSearchAlgorithms::_bin_ocr(const Dictionary &dict, double sim, std::map<point_t, ocr_rec_t> &ps) {
    int px, py;
    if (_binary.empty())
        return;
    //
    record_sum(_binary);
    // find cnt range
    // find width and height range;
    int cnt_min = 255 * 255, cnt_max = 0;
    int w_min = 255, h_min = 255;
    int w_max = 0, h_max = 0;
    for (auto &it : dict.words) {
        cnt_min = min(cnt_min, it.info.bit_cnt);
        cnt_max = max(cnt_max, it.info.bit_cnt);
        w_min = min(w_min, it.info.w);
        h_min = min(h_min, it.info.h);
        w_max = max(w_max, it.info.w);
        h_max = max(h_max, it.info.h);
    }
    // int matched = 0;
    // 遍历行
    for (py = 0; py < _binary.height - h_min + 1; ++py) {
        // 遍历列
        for (px = 0; px < _binary.width - w_min + 1; ++px) {
            if (_record.at(py, px))
                continue;
            // 检测像素密度
            if (region_sum(px, py, min(px + w_max, _binary.width), min(py + h_max, _binary.height)) <
                cnt_min * sim) // too less
                continue;
            if (region_sum(px, py, px + w_min, py + h_min) > cnt_max * (2 - sim))
                continue;
            point_t pt;
            pt.x = px;
            pt.y = py;
            // 遍历字库
            //  assert(i != 4 || j != 3);
            int k = 0;
            for (auto &it : dict.words) {
                rect_t crc;
                crc.x1 = px;
                crc.y1 = py;
                crc.x2 = px + it.info.w;
                crc.y2 = py + it.info.h;
                // 边界检查
                if (crc.y2 > _binary.height || crc.x2 > _binary.width)
                    continue;
                // quick check
                // error tolerance
                int error_tolerance = static_cast<int>((1 - sim) * it.info.w * it.info.h);
                if (abs(region_sum(crc.x1, crc.y1, crc.x2, crc.y2) - it.info.bit_cnt) > error_tolerance)
                    continue;
                // match
                int match_error = part_match(_binary, crc, error_tolerance, it.data.data());
                if (match_error <= error_tolerance) {
                    // final check
                    // check right col is empty/background
                    int rs = 0;
                    if (crc.x2 < _binary.width) {
                        for (int k = crc.y1; k < crc.y2; k++)
                            rs += _binary.at(k, crc.x2);
                    }
                    if (rs <= it.info.h / 2) {
                        ocr_rec_t ocr_res;
                        ocr_res.left_top = pt;
                        ocr_res.right_bottom = point_t(crc.x2, crc.y2);
                        ocr_res.text = it.info.name;
                        ocr_res.confidence = static_cast<float>((crc.area() - match_error) / (double)crc.area());
                        ps[pt] = ocr_res;
                        fill_rect(_record, crc);
                        // break;//words
                        break;
                    } else {
                        // not matched
                    }
                }
            } // end for words
              // if (matched)break;
        }     // end for j
    }         // end for i
}

void ImageSearchAlgorithms::bin_ocr(const Dictionary &dict, double sim, std::map<point_t, ocr_rec_t> &ps) {
    ps.clear();
    if (dict.words.empty())
        return;
    if (_binary.empty())
        return;
    _record.create(_binary.width, _binary.height);
    memset(_record.data(), 0, sizeof(uchar) * _record.width * _record.height);

    if (sim > 1.0 - 1e-5) {
        _bin_ocr(dict, ps);
    } else {
        sim = 0.5 + sim / 2;

        _bin_ocr(dict, sim, ps);
    }
}

} // namespace op::image
