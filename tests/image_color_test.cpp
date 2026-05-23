#include "test_support.h"

#include <iostream>
#include <vector>

using namespace std;
using test_support::BuildBmp32TopDown;
using test_support::PtrToWString;

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
