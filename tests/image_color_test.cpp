#include "test_support.h"

#include <iostream>
#include <utility>
#include <vector>

using namespace std;
using test_support::BuildBmp32TopDown;
using test_support::PtrToWString;

namespace {
vector<uchar> MakePixels(int width, int height, uchar b = 0xff, uchar g = 0xff, uchar r = 0xff) {
    vector<uchar> pixels(static_cast<size_t>(width) * height * 4, 0xff);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const auto idx = static_cast<size_t>(y * width + x) * 4;
            pixels[idx + 0] = b;
            pixels[idx + 1] = g;
            pixels[idx + 2] = r;
        }
    }
    return pixels;
}

void PaintPixel(vector<uchar> &pixels, int width, int x, int y, uchar b, uchar g, uchar r) {
    const auto idx = static_cast<size_t>(y * width + x) * 4;
    pixels[idx + 0] = b;
    pixels[idx + 1] = g;
    pixels[idx + 2] = r;
    pixels[idx + 3] = 0xff;
}

void PaintGlyphA(vector<uchar> &pixels, int width, int offset_x, int offset_y, uchar b, uchar g, uchar r) {
    static const vector<pair<int, int>> points = {{2, 1}, {1, 2}, {3, 2}, {1, 3},
                                                  {2, 3}, {3, 3}, {1, 4}, {3, 4}};
    for (auto [x, y] : points)
        PaintPixel(pixels, width, x + offset_x, y + offset_y, b, g, r);
}

void PaintScaledGlyphA(vector<uchar> &pixels, int width, int offset_x, int offset_y, int scale, uchar b, uchar g,
                       uchar r) {
    static const vector<pair<int, int>> points = {{2, 1}, {1, 2}, {3, 2}, {1, 3},
                                                  {2, 3}, {3, 3}, {1, 4}, {3, 4}};
    for (auto [x, y] : points) {
        for (int dy = 0; dy < scale; ++dy) {
            for (int dx = 0; dx < scale; ++dx) {
                PaintPixel(pixels, width, offset_x + x * scale + dx, offset_y + y * scale + dy, b, g, r);
            }
        }
    }
}

void PaintOutlinedGlyphA(vector<uchar> &pixels, int width, int offset_x, int offset_y, uchar outline_b,
                         uchar outline_g, uchar outline_r, uchar fill_b, uchar fill_g, uchar fill_r) {
    static const vector<pair<int, int>> points = {{2, 1}, {1, 2}, {3, 2}, {1, 3},
                                                  {2, 3}, {3, 3}, {1, 4}, {3, 4}};
    for (auto [x, y] : points) {
        for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx) {
                PaintPixel(pixels, width, offset_x + x + dx, offset_y + y + dy, outline_b, outline_g, outline_r);
            }
        }
    }
    for (auto [x, y] : points)
        PaintPixel(pixels, width, x + offset_x, y + offset_y, fill_b, fill_g, fill_r);
}

void PaintGameBackground(vector<uchar> &pixels, int width, int height) {
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const auto b = static_cast<uchar>(0x50 + (x * 17 + y * 29) % 0x50);
            const auto g = static_cast<uchar>(0x58 + (x * 31 + y * 11) % 0x48);
            const auto r = static_cast<uchar>(0x48 + (x * 13 + y * 23) % 0x58);
            PaintPixel(pixels, width, x, y, b, g, r);
        }
    }
}

void SetMemBmp(libop &op, int width, int height, const vector<uchar> &pixels, long &ret) {
    auto bmp = BuildBmp32TopDown(width, height, pixels);
    wstring mode = L"mem:" + PtrToWString(bmp.data());
    op.SetDisplayInput(mode.c_str(), &ret);
}

void UseSingleWordDict(libop &op, int width, int height, const wchar_t *color, const wchar_t *word, long &ret) {
    wstring dict_entry;
    op.FetchWord(0, 0, width, height, color, word, dict_entry);
    ASSERT_FALSE(dict_entry.empty());
    op.ClearDict(0, &ret);
    ASSERT_EQ(ret, 1);
    op.AddDict(0, dict_entry.c_str(), &ret);
    ASSERT_EQ(ret, 1);
    op.UseDict(0, &ret);
    ASSERT_EQ(ret, 1);
}
} // namespace

TEST(ImageColorTest, GetColor) {
    libop op;
    wstring color;
    op.GetColor(10, 10, color);
    wcout << L"Color at (10,10): " << color << endl;
    EXPECT_EQ(color.length(), 6u) << "Color string should be 6 hex characters";
}

TEST(ImageColorTest, CmpColor) {
    libop op;
    wstring color;
    op.GetColor(10, 10, color);
    ASSERT_EQ(color.length(), 6u);

    long ret;
    op.CmpColor(10, 10, color.c_str(), 0.9, &ret);
    EXPECT_TRUE(ret) << "CmpColor should match the same color just read";
}

TEST(ImageColorTest, CmpColorUsesSimilarityAndExplicitDelta) {
    libop op;
    long ret = 0;
    const int width = 32;
    const int height = 32;
    vector<uchar> pixels(static_cast<size_t>(width) * height * 4, 0xff);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const auto idx = static_cast<size_t>(y * width + x) * 4;
            pixels[idx + 0] = 0x20;
            pixels[idx + 1] = 0x30;
            pixels[idx + 2] = 0x40;
        }
    }
    auto bmp = BuildBmp32TopDown(width, height, pixels);
    wstring mode = L"mem:" + PtrToWString(bmp.data());

    op.SetDisplayInput(mode.c_str(), &ret);
    ASSERT_EQ(ret, 1);

    op.CmpColor(10, 10, L"433222", 1.0, &ret);
    EXPECT_EQ(ret, 0) << "Different colors should not match at exact similarity";

    op.CmpColor(10, 10, L"433222", 0.98, &ret);
    EXPECT_EQ(ret, 1) << "Similarity should provide an implicit color tolerance";

    op.CmpColor(10, 10, L"433222-030202", 1.0, &ret);
    EXPECT_EQ(ret, 1) << "Explicit delta in the color string should still be honored";
}

TEST(ImageColorTest, FindColor) {
    libop op;
    wstring color;
    op.GetColor(10, 10, color);
    ASSERT_EQ(color.length(), 6u);

    long x, y, ret;
    op.FindColor(0, 0, 100, 100, color.c_str(), 0.9, 0, &x, &y, &ret);
    cout << "FindColor ret: " << ret << " at " << x << "," << y << endl;
}

TEST(ImageColorTest, FindColorUsesSimilarity) {
    libop op;
    long ret = 0;
    const int width = 16;
    const int height = 16;
    vector<uchar> pixels(static_cast<size_t>(width) * height * 4, 0xff);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const auto idx = static_cast<size_t>(y * width + x) * 4;
            pixels[idx + 0] = 0x00;
            pixels[idx + 1] = 0x00;
            pixels[idx + 2] = 0x00;
        }
    }

    const auto marker_idx = static_cast<size_t>(8 * width + 7) * 4;
    pixels[marker_idx + 0] = 0x20;
    pixels[marker_idx + 1] = 0x30;
    pixels[marker_idx + 2] = 0x40;

    auto bmp = BuildBmp32TopDown(width, height, pixels);
    wstring mode = L"mem:" + PtrToWString(bmp.data());

    op.SetDisplayInput(mode.c_str(), &ret);
    ASSERT_EQ(ret, 1);

    long x = -1;
    long y = -1;
    op.FindColor(0, 0, width, height, L"433222", 1.0, 0, &x, &y, &ret);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(x, -1);
    EXPECT_EQ(y, -1);

    op.FindColor(0, 0, width, height, L"433222", 0.98, 0, &x, &y, &ret);
    EXPECT_EQ(ret, 1);
    EXPECT_EQ(x, 7);
    EXPECT_EQ(y, 8);
}

TEST(ImageColorTest, FindColorHonorsAllDirections) {
    libop op;
    long ret = 0;
    const int width = 8;
    const int height = 8;
    vector<uchar> pixels(static_cast<size_t>(width) * height * 4, 0xff);

    auto paint_marker = [&](int x, int y) {
        const auto idx = static_cast<size_t>(y * width + x) * 4;
        pixels[idx + 0] = 0x11;
        pixels[idx + 1] = 0x22;
        pixels[idx + 2] = 0x33;
        pixels[idx + 3] = 0xff;
    };
    paint_marker(1, 1);
    paint_marker(6, 1);
    paint_marker(3, 3);
    paint_marker(1, 6);
    paint_marker(6, 6);

    auto bmp = BuildBmp32TopDown(width, height, pixels);
    wstring mode = L"mem:" + PtrToWString(bmp.data());

    op.SetDisplayInput(mode.c_str(), &ret);
    ASSERT_EQ(ret, 1);

    const vector<wstring> expected = {
        L"1,1|6,1|3,3|1,6|6,6",
        L"1,6|6,6|3,3|1,1|6,1",
        L"6,1|1,1|3,3|6,6|1,6",
        L"6,6|1,6|3,3|6,1|1,1",
        L"3,3|1,1|6,1|1,6|6,6",
        L"1,1|1,6|3,3|6,1|6,6",
        L"6,1|6,6|3,3|1,1|1,6",
        L"1,6|1,1|3,3|6,6|6,1",
        L"6,6|6,1|3,3|1,6|1,1",
    };
    const vector<long> expected_x = {1, 1, 6, 6, 3, 1, 6, 1, 6};
    const vector<long> expected_y = {1, 6, 1, 6, 3, 1, 1, 6, 6};

    for (long dir = 0; dir <= 8; ++dir) {
        long x = -1;
        long y = -1;
        op.FindColor(0, 0, width, height, L"332211", 1.0, dir, &x, &y, &ret);
        ASSERT_EQ(ret, 1) << "dir=" << dir;
        EXPECT_EQ(x, expected_x[dir]) << "dir=" << dir;
        EXPECT_EQ(y, expected_y[dir]) << "dir=" << dir;

        wstring results;
        op.FindColorEx(0, 0, width, height, L"332211", 1.0, dir, results);
        EXPECT_EQ(results, expected[dir]) << "dir=" << dir;
    }
}

TEST(ImageColorTest, FindMultiColorHonorsAllDirections) {
    libop op;
    long ret = 0;
    const int width = 8;
    const int height = 8;
    vector<uchar> pixels(static_cast<size_t>(width) * height * 4, 0xff);

    auto paint = [&](int x, int y, uchar b, uchar g, uchar r) {
        const auto idx = static_cast<size_t>(y * width + x) * 4;
        pixels[idx + 0] = b;
        pixels[idx + 1] = g;
        pixels[idx + 2] = r;
        pixels[idx + 3] = 0xff;
    };
    auto paint_marker = [&](int x, int y) {
        paint(x - 1, y, 0x44, 0x55, 0x66);
        paint(x, y, 0x11, 0x22, 0x33);
    };
    paint_marker(1, 1);
    paint_marker(6, 1);
    paint_marker(3, 3);
    paint_marker(1, 6);
    paint_marker(6, 6);

    auto bmp = BuildBmp32TopDown(width, height, pixels);
    wstring mode = L"mem:" + PtrToWString(bmp.data());

    op.SetDisplayInput(mode.c_str(), &ret);
    ASSERT_EQ(ret, 1);

    const vector<wstring> expected = {
        L"1,1|6,1|3,3|1,6|6,6",
        L"1,6|6,6|3,3|1,1|6,1",
        L"6,1|1,1|3,3|6,6|1,6",
        L"6,6|1,6|3,3|6,1|1,1",
        L"3,3|1,1|6,1|1,6|6,6",
        L"1,1|1,6|3,3|6,1|6,6",
        L"6,1|6,6|3,3|1,1|1,6",
        L"1,6|1,1|3,3|6,6|6,1",
        L"6,6|6,1|3,3|1,6|1,1",
    };
    const vector<long> expected_x = {1, 1, 6, 6, 3, 1, 6, 1, 6};
    const vector<long> expected_y = {1, 6, 1, 6, 3, 1, 1, 6, 6};

    for (long dir = 0; dir <= 8; ++dir) {
        long x = -1;
        long y = -1;
        op.FindMultiColor(0, 0, width, height, L"332211", L"-1|0|665544", 1.0, dir, &x, &y, &ret);
        ASSERT_EQ(ret, 1) << "dir=" << dir;
        EXPECT_EQ(x, expected_x[dir]) << "dir=" << dir;
        EXPECT_EQ(y, expected_y[dir]) << "dir=" << dir;

        wstring results;
        op.FindMultiColorEx(0, 0, width, height, L"332211", L"-1|0|665544", 1.0, dir, results);
        EXPECT_EQ(results, expected[dir]) << "dir=" << dir;
    }
}

TEST(ImageColorTest, FindPicHonorsDirection) {
    libop op;
    long ret = 0;
    const int width = 32;
    const int height = 32;
    vector<uchar> pixels(static_cast<size_t>(width) * height * 4, 0xff);

    auto paint = [&](int x, int y, uchar b, uchar g, uchar r) {
        const auto idx = static_cast<size_t>(y * width + x) * 4;
        pixels[idx + 0] = b;
        pixels[idx + 1] = g;
        pixels[idx + 2] = r;
        pixels[idx + 3] = 0xff;
    };
    auto paint_marker = [&](int left, int top) {
        paint(left, top, 0x11, 0x22, 0x33);
        paint(left + 1, top, 0x44, 0x55, 0x66);
        paint(left, top + 1, 0x77, 0x88, 0x99);
        paint(left + 1, top + 1, 0xaa, 0xbb, 0xcc);
    };

    paint_marker(2, 3);
    paint_marker(24, 3);
    paint_marker(15, 15);
    paint_marker(2, 25);
    paint_marker(24, 25);
    auto bmp = BuildBmp32TopDown(width, height, pixels);
    wstring mode = L"mem:" + PtrToWString(bmp.data());

    op.SetDisplayInput(mode.c_str(), &ret);
    ASSERT_EQ(ret, 1);

    vector<uchar> tpl(static_cast<size_t>(2) * 2 * 4, 0xff);
    auto write_tpl = [&](int x, int y, uchar b, uchar g, uchar r) {
        const auto idx = static_cast<size_t>(y * 2 + x) * 4;
        tpl[idx + 0] = b;
        tpl[idx + 1] = g;
        tpl[idx + 2] = r;
        tpl[idx + 3] = 0xff;
    };
    write_tpl(0, 0, 0x11, 0x22, 0x33);
    write_tpl(1, 0, 0x44, 0x55, 0x66);
    write_tpl(0, 1, 0x77, 0x88, 0x99);
    write_tpl(1, 1, 0xaa, 0xbb, 0xcc);
    auto tpl_bmp = BuildBmp32TopDown(2, 2, tpl);
    op.LoadMemPic(L"findpic_dir_marker", tpl_bmp.data(), static_cast<long>(tpl_bmp.size()), &ret);
    ASSERT_EQ(ret, 1);

    const vector<wstring> expected = {
        L"0,2,3|0,24,3|0,15,15|0,2,25|0,24,25",
        L"0,2,25|0,24,25|0,15,15|0,2,3|0,24,3",
        L"0,24,3|0,2,3|0,15,15|0,24,25|0,2,25",
        L"0,24,25|0,2,25|0,15,15|0,24,3|0,2,3",
        L"0,15,15|0,24,25|0,24,3|0,2,25|0,2,3",
        L"0,2,3|0,2,25|0,15,15|0,24,3|0,24,25",
        L"0,24,3|0,24,25|0,15,15|0,2,3|0,2,25",
        L"0,2,25|0,2,3|0,15,15|0,24,25|0,24,3",
        L"0,24,25|0,24,3|0,15,15|0,2,25|0,2,3",
    };
    const vector<long> expected_x = {2, 2, 24, 24, 15, 2, 24, 2, 24};
    const vector<long> expected_y = {3, 25, 3, 25, 15, 3, 3, 25, 25};

    auto named_results = [](const wstring &id_results) {
        wstring named;
        size_t start = 0;
        while (start < id_results.size()) {
            const size_t end = id_results.find(L'|', start);
            const wstring item = id_results.substr(start, end == wstring::npos ? wstring::npos : end - start);
            const size_t comma = item.find(L',');
            named += L"findpic_dir_marker" + item.substr(comma) + L"|";
            if (end == wstring::npos)
                break;
            start = end + 1;
        }
        if (!named.empty())
            named.pop_back();
        return named;
    };

    for (long dir = 0; dir <= 8; ++dir) {
        long x = -1;
        long y = -1;
        op.FindPic(0, 0, width, height, L"findpic_dir_marker", L"000000", 1.0, dir, &x, &y, &ret);
        ASSERT_EQ(ret, 0) << "dir=" << dir;
        EXPECT_EQ(x, expected_x[dir]) << "dir=" << dir;
        EXPECT_EQ(y, expected_y[dir]) << "dir=" << dir;

        wstring results;
        op.FindPicEx(0, 0, width, height, L"findpic_dir_marker", L"000000", 1.0, dir, results);
        EXPECT_EQ(results, expected[dir]) << "dir=" << dir;

        op.FindPicExS(0, 0, width, height, L"findpic_dir_marker", L"000000", 1.0, dir, results);
        EXPECT_EQ(results, named_results(expected[dir])) << "dir=" << dir;
    }
}

TEST(ImageColorTest, FetchWordUsesProvidedWordName) {
    libop op;
    long ret = 0;
    const int width = 8;
    const int height = 8;
    vector<uchar> pixels(static_cast<size_t>(width) * height * 4, 0xff);

    auto paint = [&](int x, int y) {
        const auto idx = static_cast<size_t>(y * width + x) * 4;
        pixels[idx + 0] = 0x00;
        pixels[idx + 1] = 0x00;
        pixels[idx + 2] = 0x00;
        pixels[idx + 3] = 0xff;
    };

    // A small glyph-like block so FetchWord has foreground pixels to crop.
    paint(2, 1);
    paint(1, 2);
    paint(3, 2);
    paint(1, 3);
    paint(2, 3);
    paint(3, 3);
    paint(1, 4);
    paint(3, 4);

    auto bmp = BuildBmp32TopDown(width, height, pixels);
    wstring mode = L"mem:" + PtrToWString(bmp.data());

    op.SetDisplayInput(mode.c_str(), &ret);
    ASSERT_EQ(ret, 1);

    wstring word_data;
    op.FetchWord(0, 0, width, height, L"000000-000000", L"A", word_data);

    wcout << L"FetchWord output: " << word_data << endl;
    EXPECT_FALSE(word_data.empty());
    EXPECT_EQ(word_data.rfind(L"A$", 0), 0u) << "FetchWord should preserve the provided word name";

    op.FetchWord(1, 1, 4, 5, L"000000-000000", L"B", word_data);
    wcout << L"FetchWord offset output: " << word_data << endl;
    EXPECT_EQ(word_data, L"B$4,3,8$5E0E") << "FetchWord should use local coordinates after capturing an offset rect";
}

TEST(ImageColorTest, FetchWordReturnsEmptyForBlankRegion) {
    libop op;
    long ret = 0;
    const int width = 8;
    const int height = 8;
    vector<uchar> pixels(static_cast<size_t>(width) * height * 4, 0xff);
    auto bmp = BuildBmp32TopDown(width, height, pixels);
    wstring mode = L"mem:" + PtrToWString(bmp.data());

    op.SetDisplayInput(mode.c_str(), &ret);
    ASSERT_EQ(ret, 1);

    wstring word_data;
    op.FetchWord(0, 0, width, height, L"000000-000000", L"Blank", word_data);

    EXPECT_TRUE(word_data.empty()) << "FetchWord should not emit a dictionary entry for a blank region";
}

TEST(ImageColorTest, FindStrUsesChannelColorToleranceForBinaryText) {
    libop op;
    long ret = 0;
    const int width = 8;
    const int height = 8;

    auto make_pixels = [&](bool add_blue_noise) {
        vector<uchar> pixels(static_cast<size_t>(width) * height * 4, 0xff);
        auto paint = [&](int x, int y, uchar b, uchar g, uchar r) {
            const auto idx = static_cast<size_t>(y * width + x) * 4;
            pixels[idx + 0] = b;
            pixels[idx + 1] = g;
            pixels[idx + 2] = r;
            pixels[idx + 3] = 0xff;
        };
        for (auto [x, y] : {pair{2, 1}, pair{1, 2}, pair{3, 2}, pair{1, 3}, pair{2, 3}, pair{3, 3}, pair{1, 4},
                            pair{3, 4}}) {
            paint(x, y, 0x00, 0x00, 0x00);
        }
        if (add_blue_noise)
            paint(2, 2, 0xff, 0x00, 0x00);
        return pixels;
    };

    auto clean_bmp = BuildBmp32TopDown(width, height, make_pixels(false));
    wstring mode = L"mem:" + PtrToWString(clean_bmp.data());
    op.SetDisplayInput(mode.c_str(), &ret);
    ASSERT_EQ(ret, 1);

    wstring dict_entry;
    op.FetchWord(0, 0, width, height, L"000000-303030", L"A", dict_entry);
    ASSERT_EQ(dict_entry, L"A$4,3,8$5E0E");
    op.ClearDict(0, &ret);
    ASSERT_EQ(ret, 1);
    op.AddDict(0, dict_entry.c_str(), &ret);
    ASSERT_EQ(ret, 1);
    op.UseDict(0, &ret);
    ASSERT_EQ(ret, 1);

    auto noisy_bmp = BuildBmp32TopDown(width, height, make_pixels(true));
    mode = L"mem:" + PtrToWString(noisy_bmp.data());
    op.SetDisplayInput(mode.c_str(), &ret);
    ASSERT_EQ(ret, 1);

    long x = -1;
    long y = -1;
    op.FindStr(0, 0, width, height, L"A", L"000000-303030", 1.0, &x, &y, &ret);
    EXPECT_EQ(ret, 0) << "Blue noise must not match the black text tolerance";
    EXPECT_EQ(x, 1);
    EXPECT_EQ(y, 1);
}

TEST(ImageColorTest, FindStrKeepsAntialiasedTextWithinTolerance) {
    libop op;
    long ret = 0;
    const int width = 8;
    const int height = 8;

    auto clean_pixels = MakePixels(width, height);
    PaintGlyphA(clean_pixels, width, 0, 0, 0x00, 0x00, 0x00);
    SetMemBmp(op, width, height, clean_pixels, ret);
    ASSERT_EQ(ret, 1);

    wstring dict_entry;
    op.FetchWord(0, 0, width, height, L"000000-000000", L"A", dict_entry);
    ASSERT_EQ(dict_entry, L"A$4,3,8$5E0E");
    op.ClearDict(0, &ret);
    ASSERT_EQ(ret, 1);
    op.AddDict(0, dict_entry.c_str(), &ret);
    ASSERT_EQ(ret, 1);
    op.UseDict(0, &ret);
    ASSERT_EQ(ret, 1);

    auto antialiased_pixels = MakePixels(width, height);
    PaintGlyphA(antialiased_pixels, width, 0, 0, 0x22, 0x22, 0x22);
    SetMemBmp(op, width, height, antialiased_pixels, ret);
    ASSERT_EQ(ret, 1);

    long x = -1;
    long y = -1;
    op.FindStr(0, 0, width, height, L"A", L"000000-303030", 1.0, &x, &y, &ret);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(x, 1);
    EXPECT_EQ(y, 1);
}

TEST(ImageColorTest, FindStrSupportsColoredTextWithChannelTolerance) {
    libop op;
    long ret = 0;
    const int width = 8;
    const int height = 8;

    auto clean_pixels = MakePixels(width, height);
    PaintGlyphA(clean_pixels, width, 0, 0, 0x00, 0x00, 0xff);
    SetMemBmp(op, width, height, clean_pixels, ret);
    ASSERT_EQ(ret, 1);

    wstring dict_entry;
    op.FetchWord(0, 0, width, height, L"FF0000-000000", L"A", dict_entry);
    ASSERT_EQ(dict_entry, L"A$4,3,8$5E0E");
    op.ClearDict(0, &ret);
    ASSERT_EQ(ret, 1);
    op.AddDict(0, dict_entry.c_str(), &ret);
    ASSERT_EQ(ret, 1);
    op.UseDict(0, &ret);
    ASSERT_EQ(ret, 1);

    auto noisy_pixels = MakePixels(width, height);
    PaintGlyphA(noisy_pixels, width, 0, 0, 0x10, 0x12, 0xe8);
    PaintPixel(noisy_pixels, width, 2, 2, 0x00, 0x80, 0x00);
    SetMemBmp(op, width, height, noisy_pixels, ret);
    ASSERT_EQ(ret, 1);

    long x = -1;
    long y = -1;
    op.FindStr(0, 0, width, height, L"A", L"FF0000-202020", 1.0, &x, &y, &ret);
    EXPECT_EQ(ret, 0) << "Same-gray green noise must not match red text tolerance";
    EXPECT_EQ(x, 1);
    EXPECT_EQ(y, 1);
}

TEST(ImageColorTest, FindStrSupportsMultipleForegroundColors) {
    libop op;
    long ret = 0;
    const int width = 8;
    const int height = 8;

    auto paint_mixed_glyph = [&](vector<uchar> &pixels, uchar red_b, uchar red_g, uchar red_r, uchar black) {
        PaintPixel(pixels, width, 2, 1, red_b, red_g, red_r);
        PaintPixel(pixels, width, 1, 2, black, black, black);
        PaintPixel(pixels, width, 3, 2, red_b, red_g, red_r);
        PaintPixel(pixels, width, 1, 3, black, black, black);
        PaintPixel(pixels, width, 2, 3, red_b, red_g, red_r);
        PaintPixel(pixels, width, 3, 3, black, black, black);
        PaintPixel(pixels, width, 1, 4, red_b, red_g, red_r);
        PaintPixel(pixels, width, 3, 4, black, black, black);
    };

    auto clean_pixels = MakePixels(width, height);
    paint_mixed_glyph(clean_pixels, 0x00, 0x00, 0xff, 0x00);
    SetMemBmp(op, width, height, clean_pixels, ret);
    ASSERT_EQ(ret, 1);

    wstring dict_entry;
    op.FetchWord(0, 0, width, height, L"000000-000000|FF0000-000000", L"A", dict_entry);
    ASSERT_EQ(dict_entry, L"A$4,3,8$5E0E");
    op.ClearDict(0, &ret);
    ASSERT_EQ(ret, 1);
    op.AddDict(0, dict_entry.c_str(), &ret);
    ASSERT_EQ(ret, 1);
    op.UseDict(0, &ret);
    ASSERT_EQ(ret, 1);

    auto tolerant_pixels = MakePixels(width, height);
    paint_mixed_glyph(tolerant_pixels, 0x08, 0x10, 0xf0, 0x18);
    SetMemBmp(op, width, height, tolerant_pixels, ret);
    ASSERT_EQ(ret, 1);

    long x = -1;
    long y = -1;
    op.FindStr(0, 0, width, height, L"A", L"000000-202020|FF0000-202020", 1.0, &x, &y, &ret);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(x, 1);
    EXPECT_EQ(y, 1);
}

TEST(ImageColorTest, LocalOcrApisUseBinaryColorTolerance) {
    libop op;
    long ret = 0;
    const int width = 16;
    const int height = 8;

    auto dict_pixels = MakePixels(8, height);
    PaintGlyphA(dict_pixels, 8, 0, 0, 0x00, 0x00, 0x00);
    SetMemBmp(op, 8, height, dict_pixels, ret);
    ASSERT_EQ(ret, 1);

    wstring dict_entry;
    op.FetchWord(0, 0, 8, height, L"000000-000000", L"A", dict_entry);
    ASSERT_EQ(dict_entry, L"A$4,3,8$5E0E");
    op.ClearDict(0, &ret);
    ASSERT_EQ(ret, 1);
    op.AddDict(0, dict_entry.c_str(), &ret);
    ASSERT_EQ(ret, 1);
    op.UseDict(0, &ret);
    ASSERT_EQ(ret, 1);

    auto pixels = MakePixels(width, height);
    PaintGlyphA(pixels, width, 0, 0, 0x20, 0x20, 0x20);
    PaintGlyphA(pixels, width, 8, 0, 0x20, 0x20, 0x20);
    SetMemBmp(op, width, height, pixels, ret);
    ASSERT_EQ(ret, 1);

    wstring str;
    op.Ocr(0, 0, width, height, L"000000-303030", 1.0, str);
    EXPECT_EQ(str, L"AA");

    op.OcrEx(0, 0, width, height, L"000000-303030", 1.0, str);
    EXPECT_EQ(str, L"1,1,A|9,1,A");

    op.FindStrEx(0, 0, width, height, L"A", L"000000-303030", 1.0, str);
    EXPECT_EQ(str, L"0,1,1|0,9,1");
}

TEST(ImageColorTest, PointTextUsesSimilarityAsImplicitColorTolerance) {
    libop op;
    long ret = 0;
    const int width = 16;
    const int height = 8;

    auto dict_pixels = MakePixels(8, height);
    PaintGlyphA(dict_pixels, 8, 0, 0, 0x00, 0x00, 0x00);
    SetMemBmp(op, 8, height, dict_pixels, ret);
    ASSERT_EQ(ret, 1);

    wstring dict_entry;
    op.FetchWord(0, 0, 8, height, L"000000-000000", L"A", dict_entry);
    ASSERT_EQ(dict_entry, L"A$4,3,8$5E0E");
    op.ClearDict(0, &ret);
    ASSERT_EQ(ret, 1);
    op.AddDict(0, dict_entry.c_str(), &ret);
    ASSERT_EQ(ret, 1);
    op.UseDict(0, &ret);
    ASSERT_EQ(ret, 1);

    auto pixels = MakePixels(width, height);
    PaintGlyphA(pixels, width, 0, 0, 0x19, 0x19, 0x19);
    PaintGlyphA(pixels, width, 8, 0, 0x19, 0x19, 0x19);
    SetMemBmp(op, width, height, pixels, ret);
    ASSERT_EQ(ret, 1);

    long x = -1;
    long y = -1;
    op.FindStr(0, 0, width, height, L"A", L"000000", 1.0, &x, &y, &ret);
    EXPECT_EQ(ret, -1) << "Exact similarity keeps color matching exact when no explicit delta is provided";
    EXPECT_EQ(x, -1);
    EXPECT_EQ(y, -1);

    op.FindStr(0, 0, width, height, L"A", L"000000", 0.9, &x, &y, &ret);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(x, 1);
    EXPECT_EQ(y, 1);

    wstring str;
    op.Ocr(0, 0, width, height, L"000000", 0.9, str);
    EXPECT_EQ(str, L"AA");

    op.OcrEx(0, 0, width, height, L"000000", 0.9, str);
    EXPECT_EQ(str, L"1,1,A|9,1,A");

    op.FindStrEx(0, 0, width, height, L"A", L"000000", 0.9, str);
    EXPECT_EQ(str, L"0,1,1|0,9,1");
}

TEST(ImageColorTest, PointTextExplicitZeroDeltaOverridesSimilarityTolerance) {
    libop op;
    long ret = 0;
    const int width = 8;
    const int height = 8;

    auto dict_pixels = MakePixels(width, height);
    PaintGlyphA(dict_pixels, width, 0, 0, 0x00, 0x00, 0x00);
    SetMemBmp(op, width, height, dict_pixels, ret);
    ASSERT_EQ(ret, 1);

    wstring dict_entry;
    op.FetchWord(0, 0, width, height, L"000000-000000", L"A", dict_entry);
    ASSERT_EQ(dict_entry, L"A$4,3,8$5E0E");
    op.ClearDict(0, &ret);
    ASSERT_EQ(ret, 1);
    op.AddDict(0, dict_entry.c_str(), &ret);
    ASSERT_EQ(ret, 1);
    op.UseDict(0, &ret);
    ASSERT_EQ(ret, 1);

    auto pixels = MakePixels(width, height);
    PaintGlyphA(pixels, width, 0, 0, 0x19, 0x19, 0x19);
    SetMemBmp(op, width, height, pixels, ret);
    ASSERT_EQ(ret, 1);

    long x = -1;
    long y = -1;
    op.FindStr(0, 0, width, height, L"A", L"000000-000000", 0.9, &x, &y, &ret);
    EXPECT_EQ(ret, -1) << "Explicit -000000 keeps point-text color matching exact";
    EXPECT_EQ(x, -1);
    EXPECT_EQ(y, -1);
}

TEST(ImageColorTest, FetchWordTreatsMultipleBackgroundColorsAsBackground) {
    libop op;
    long ret = 0;
    const int width = 8;
    const int height = 8;

    auto pixels = MakePixels(width, height);
    for (int y = 0; y < height; ++y) {
        for (int x = width / 2; x < width; ++x) {
            PaintPixel(pixels, width, x, y, 0xee, 0xee, 0xee);
        }
    }
    SetMemBmp(op, width, height, pixels, ret);
    ASSERT_EQ(ret, 1);

    wstring word_data;
    op.FetchWord(0, 0, width, height, L"@FFFFFF-000000|EEEEEE-000000", L"Blank", word_data);

    EXPECT_TRUE(word_data.empty()) << "All pixels are declared background colors, so no word should be extracted";
}

TEST(ImageColorTest, FindStrSupportsAutoBackgroundBinarization) {
    libop op;
    long ret = 0;
    const int width = 8;
    const int height = 8;

    auto clean_pixels = MakePixels(width, height);
    PaintGlyphA(clean_pixels, width, 0, 0, 0x00, 0x00, 0x00);
    SetMemBmp(op, width, height, clean_pixels, ret);
    ASSERT_EQ(ret, 1);

    wstring dict_entry;
    op.FetchWord(0, 0, width, height, L"000000-000000", L"A", dict_entry);
    ASSERT_EQ(dict_entry, L"A$4,3,8$5E0E");
    op.ClearDict(0, &ret);
    ASSERT_EQ(ret, 1);
    op.AddDict(0, dict_entry.c_str(), &ret);
    ASSERT_EQ(ret, 1);
    op.UseDict(0, &ret);
    ASSERT_EQ(ret, 1);

    auto pixels = MakePixels(width, height);
    PaintGlyphA(pixels, width, 0, 0, 0x00, 0x00, 0x00);
    SetMemBmp(op, width, height, pixels, ret);
    ASSERT_EQ(ret, 1);

    long x = -1;
    long y = -1;
    op.FindStr(0, 0, width, height, L"A", L"", 1.0, &x, &y, &ret);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(x, 1);
    EXPECT_EQ(y, 1);
}

TEST(ImageColorTest, GameHudFindsBrightTextOnDynamicBackground) {
    libop op;
    long ret = 0;
    const int dict_width = 8;
    const int dict_height = 8;

    auto dict_pixels = MakePixels(dict_width, dict_height, 0x40, 0x40, 0x40);
    PaintGlyphA(dict_pixels, dict_width, 0, 0, 0xff, 0xff, 0xff);
    SetMemBmp(op, dict_width, dict_height, dict_pixels, ret);
    ASSERT_EQ(ret, 1);
    ASSERT_NO_FATAL_FAILURE(UseSingleWordDict(op, dict_width, dict_height, L"FFFFFF-000000", L"A", ret));

    const int width = 32;
    const int height = 18;
    auto pixels = MakePixels(width, height, 0x40, 0x50, 0x60);
    PaintGlyphA(pixels, width, 12, 6, 0xf2, 0xf5, 0xff);
    SetMemBmp(op, width, height, pixels, ret);
    ASSERT_EQ(ret, 1);

    long x = -1;
    long y = -1;
    op.FindStr(0, 0, width, height, L"A", L"FFFFFF-202020", 1.0, &x, &y, &ret);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(x, 13);
    EXPECT_EQ(y, 7);
}

TEST(ImageColorTest, GameHudFindsOutlinedTextWithMultipleForegroundColors) {
    libop op;
    long ret = 0;
    const int dict_width = 10;
    const int dict_height = 10;

    auto dict_pixels = MakePixels(dict_width, dict_height, 0x50, 0x60, 0x70);
    PaintOutlinedGlyphA(dict_pixels, dict_width, 2, 2, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff);
    SetMemBmp(op, dict_width, dict_height, dict_pixels, ret);
    ASSERT_EQ(ret, 1);
    ASSERT_NO_FATAL_FAILURE(
        UseSingleWordDict(op, dict_width, dict_height, L"FFFFFF-000000|000000-000000", L"A", ret));

    const int width = 36;
    const int height = 20;
    auto pixels = MakePixels(width, height);
    PaintGameBackground(pixels, width, height);
    PaintOutlinedGlyphA(pixels, width, 14, 5, 0x08, 0x08, 0x08, 0xee, 0xf0, 0xff);
    SetMemBmp(op, width, height, pixels, ret);
    ASSERT_EQ(ret, 1);

    long x = -1;
    long y = -1;
    op.FindStr(0, 0, width, height, L"A", L"FFFFFF-202020|000000-202020", 1.0, &x, &y, &ret);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(x, 14);
    EXPECT_EQ(y, 5);
}

TEST(ImageColorTest, GameHudIgnoresGlowAroundText) {
    libop op;
    long ret = 0;
    const int dict_width = 8;
    const int dict_height = 8;

    auto dict_pixels = MakePixels(dict_width, dict_height, 0x40, 0x40, 0x40);
    PaintGlyphA(dict_pixels, dict_width, 0, 0, 0x60, 0xe0, 0xff);
    SetMemBmp(op, dict_width, dict_height, dict_pixels, ret);
    ASSERT_EQ(ret, 1);
    ASSERT_NO_FATAL_FAILURE(UseSingleWordDict(op, dict_width, dict_height, L"FFE060-000000", L"A", ret));

    const int width = 28;
    const int height = 16;
    auto pixels = MakePixels(width, height);
    PaintGameBackground(pixels, width, height);
    PaintOutlinedGlyphA(pixels, width, 10, 4, 0x20, 0x60, 0xa0, 0x70, 0xd4, 0xf4);
    SetMemBmp(op, width, height, pixels, ret);
    ASSERT_EQ(ret, 1);

    long x = -1;
    long y = -1;
    op.FindStr(0, 0, width, height, L"A", L"FFE060-303030", 1.0, &x, &y, &ret);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(x, 11);
    EXPECT_EQ(y, 5);
}

TEST(ImageColorTest, GameHudFindsGradientDamageTextWithinTolerance) {
    libop op;
    long ret = 0;
    const int dict_width = 8;
    const int dict_height = 8;

    auto dict_pixels = MakePixels(dict_width, dict_height, 0x40, 0x40, 0x40);
    PaintGlyphA(dict_pixels, dict_width, 0, 0, 0x00, 0xaa, 0xff);
    SetMemBmp(op, dict_width, dict_height, dict_pixels, ret);
    ASSERT_EQ(ret, 1);
    ASSERT_NO_FATAL_FAILURE(UseSingleWordDict(op, dict_width, dict_height, L"FFAA00-000000", L"A", ret));

    const int width = 30;
    const int height = 18;
    auto pixels = MakePixels(width, height);
    PaintGameBackground(pixels, width, height);
    const vector<pair<int, int>> points = {{2, 1}, {1, 2}, {3, 2}, {1, 3},
                                           {2, 3}, {3, 3}, {1, 4}, {3, 4}};
    int idx = 0;
    for (auto [x, y] : points) {
        const auto b = static_cast<uchar>((idx % 3) * 0x10);
        const auto g = static_cast<uchar>(0x92 + (idx % 4) * 0x12);
        const auto r = static_cast<uchar>(0xe8 + (idx % 2) * 0x10);
        PaintPixel(pixels, width, x + 13, y + 6, b, g, r);
        ++idx;
    }
    SetMemBmp(op, width, height, pixels, ret);
    ASSERT_EQ(ret, 1);

    long x = -1;
    long y = -1;
    op.FindStr(0, 0, width, height, L"A", L"FFAA00-305030", 1.0, &x, &y, &ret);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(x, 14);
    EXPECT_EQ(y, 7);
}

TEST(ImageColorTest, GameHudFindsScaledUiTextWhenDictionaryMatchesScale) {
    libop op;
    long ret = 0;
    const int dict_width = 14;
    const int dict_height = 14;

    auto dict_pixels = MakePixels(dict_width, dict_height, 0x40, 0x40, 0x40);
    PaintScaledGlyphA(dict_pixels, dict_width, 1, 1, 2, 0xff, 0xff, 0xff);
    SetMemBmp(op, dict_width, dict_height, dict_pixels, ret);
    ASSERT_EQ(ret, 1);
    ASSERT_NO_FATAL_FAILURE(UseSingleWordDict(op, dict_width, dict_height, L"FFFFFF-000000", L"A", ret));

    const int width = 40;
    const int height = 22;
    auto pixels = MakePixels(width, height);
    PaintGameBackground(pixels, width, height);
    PaintScaledGlyphA(pixels, width, 16, 6, 2, 0xf0, 0xf0, 0xff);
    SetMemBmp(op, width, height, pixels, ret);
    ASSERT_EQ(ret, 1);

    long x = -1;
    long y = -1;
    op.FindStr(0, 0, width, height, L"A", L"FFFFFF-202020", 1.0, &x, &y, &ret);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(x, 18);
    EXPECT_EQ(y, 8);
}

TEST(ImageColorTest, GameHudScaleMismatchDoesNotMatchUnscaledDictionary) {
    libop op;
    long ret = 0;
    const int dict_width = 8;
    const int dict_height = 8;

    auto dict_pixels = MakePixels(dict_width, dict_height, 0x40, 0x40, 0x40);
    PaintGlyphA(dict_pixels, dict_width, 0, 0, 0xff, 0xff, 0xff);
    SetMemBmp(op, dict_width, dict_height, dict_pixels, ret);
    ASSERT_EQ(ret, 1);
    ASSERT_NO_FATAL_FAILURE(UseSingleWordDict(op, dict_width, dict_height, L"FFFFFF-000000", L"A", ret));

    const int width = 40;
    const int height = 22;
    auto pixels = MakePixels(width, height);
    PaintGameBackground(pixels, width, height);
    PaintScaledGlyphA(pixels, width, 16, 6, 2, 0xf0, 0xf0, 0xff);
    SetMemBmp(op, width, height, pixels, ret);
    ASSERT_EQ(ret, 1);

    long x = -1;
    long y = -1;
    op.FindStr(0, 0, width, height, L"A", L"FFFFFF-202020", 1.0, &x, &y, &ret);
    EXPECT_EQ(ret, -1) << "Point-matrix matching requires a dictionary captured at the same UI scale";
    EXPECT_EQ(x, -1);
    EXPECT_EQ(y, -1);
}

TEST(ImageColorTest, GameHudBackgroundModeSupportsTwoTonePanels) {
    libop op;
    long ret = 0;
    const int dict_width = 12;
    const int dict_height = 10;

    auto dict_pixels = MakePixels(dict_width, dict_height, 0x30, 0x40, 0x50);
    for (int y = 0; y < dict_height; ++y) {
        for (int x = dict_width / 2; x < dict_width; ++x) {
            PaintPixel(dict_pixels, dict_width, x, y, 0x38, 0x48, 0x58);
        }
    }
    PaintGlyphA(dict_pixels, dict_width, 3, 2, 0xff, 0xff, 0xff);
    SetMemBmp(op, dict_width, dict_height, dict_pixels, ret);
    ASSERT_EQ(ret, 1);
    ASSERT_NO_FATAL_FAILURE(
        UseSingleWordDict(op, dict_width, dict_height, L"@504030-080808|584838-080808", L"A", ret));

    const int width = 34;
    const int height = 18;
    auto pixels = MakePixels(width, height, 0x34, 0x44, 0x54);
    for (int y = 0; y < height; ++y) {
        for (int x = width / 2; x < width; ++x) {
            PaintPixel(pixels, width, x, y, 0x3a, 0x4a, 0x5a);
        }
    }
    PaintGlyphA(pixels, width, 14, 5, 0xf0, 0xf0, 0xff);
    SetMemBmp(op, width, height, pixels, ret);
    ASSERT_EQ(ret, 1);

    long x = -1;
    long y = -1;
    op.FindStr(0, 0, width, height, L"A", L"@504030-080808|584838-080808", 1.0, &x, &y, &ret);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(x, 15);
    EXPECT_EQ(y, 6);
}

TEST(ImageColorTest, SetDisplayInputMemBmpPointer) {
    libop op;
    long ret = 0;
    const int width = 32;
    const int height = 32;
    vector<uchar> pixels(static_cast<size_t>(width) * height * 4, 0xff);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            auto idx = static_cast<size_t>(y * width + x) * 4;
            pixels[idx + 0] = 0x01;
            pixels[idx + 1] = 0x02;
            pixels[idx + 2] = 0x03;
            pixels[idx + 3] = 0xff;
        }
    }
    auto bmp = BuildBmp32TopDown(width, height, pixels);
    wstring mode = L"mem:" + PtrToWString(bmp.data());

    op.SetDisplayInput(mode.c_str(), &ret);
    ASSERT_EQ(ret, 1);

    wstring color;
    op.GetColor(10, 10, color);
    EXPECT_EQ(color, L"030201");
}

TEST(ImageColorTest, SetDisplayInputMemRawBgr) {
    libop op;
    long ret = 0;
    const int width = 32;
    const int height = 32;
    vector<uchar> raw_bgr(static_cast<size_t>(width) * height * 3, 0);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            auto idx = static_cast<size_t>(y * width + x) * 3;
            raw_bgr[idx + 0] = 0x28;
            raw_bgr[idx + 1] = 0x32;
            raw_bgr[idx + 2] = 0x3c;
        }
    }
    wstring mode = L"mem:" + PtrToWString(raw_bgr.data()) + L",32,32,bgr";

    op.SetDisplayInput(mode.c_str(), &ret);
    ASSERT_EQ(ret, 1);

    wstring color;
    op.GetColor(10, 10, color);
    EXPECT_EQ(color, L"3C3228");
}

TEST(ImageColorTest, SetDisplayInputMemRawHexPointer) {
    libop op;
    long ret = 0;
    const int width = 32;
    const int height = 32;
    vector<uchar> raw_bgra(static_cast<size_t>(width) * height * 4, 0xff);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            auto idx = static_cast<size_t>(y * width + x) * 4;
            raw_bgra[idx + 0] = 0x11;
            raw_bgra[idx + 1] = 0x22;
            raw_bgra[idx + 2] = 0x33;
            raw_bgra[idx + 3] = 0xff;
        }
    }
    wstring mode = L"mem:" + PtrToWString(raw_bgra.data(), true) + L",32,32,bgra";

    op.SetDisplayInput(mode.c_str(), &ret);
    ASSERT_EQ(ret, 1);

    wstring color;
    op.GetColor(10, 10, color);
    EXPECT_EQ(color, L"332211");
}
TEST(ImageColorTest, SetDisplayInputMemBareRawPointerFailsWithoutChangingCurrentInput) {
    libop op;
    long ret = 0;
    const int width = 8;
    const int height = 8;

    vector<uchar> pixels(static_cast<size_t>(width) * height * 4, 0xff);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            auto idx = static_cast<size_t>(y * width + x) * 4;
            pixels[idx + 0] = 0x01;
            pixels[idx + 1] = 0x02;
            pixels[idx + 2] = 0x03;
            pixels[idx + 3] = 0xff;
        }
    }
    auto bmp = BuildBmp32TopDown(width, height, pixels);
    wstring valid_mode = L"mem:" + PtrToWString(bmp.data());
    op.SetDisplayInput(valid_mode.c_str(), &ret);
    ASSERT_EQ(ret, 1);

    vector<uchar> raw_bgr(static_cast<size_t>(width) * height * 3, 0x7f);
    wstring invalid_mode = L"mem:" + PtrToWString(raw_bgr.data());
    op.SetDisplayInput(invalid_mode.c_str(), &ret);
    EXPECT_EQ(ret, 0) << "Bare raw pointers must include width/height/format metadata";

    wstring color;
    op.GetColor(1, 1, color);
    EXPECT_EQ(color, L"030201") << "Failed mem mode parsing should not clobber the previous display input";
}
