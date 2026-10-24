#include <gtest/gtest.h>

#include <algorithm>
#include <fstream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include <windows.h>

// 这条用例守的是「文档和头文件别漂」。
//
// include/libop.h 里 class OP_API Op 的每个公开方法，都必须在 doc/api/*.md 里有一条
// 带签名的记录；反过来，文档里提到的每个方法名也得真在头文件里存在。
// 只写了新方法没跑 scripts/gen_api_doc.py，或者文档里手写了个错名字，都会在这里失败。
//
// 文档目录找不到时跳过（发布包里不带 doc/），不把包装环境当成缺陷。

namespace {

// ctest 的工作目录是 build/vs2022-<arch>-Release/tests/Release，往上四级才是仓库根。
// 不写死层数，改成逐级向上找「include/libop.h 与 doc/api 同时存在」的目录。
std::wstring FindRepoRoot() {
    wchar_t buffer[MAX_PATH * 2] = {0};
    const DWORD length = ::GetCurrentDirectoryW(_countof(buffer), buffer);
    if (length == 0 || length >= _countof(buffer)) {
        return std::wstring();
    }

    std::wstring dir(buffer, length);
    for (int level = 0; level < 6; ++level) {
        const std::wstring header = dir + L"\\include\\libop.h";
        const std::wstring docs = dir + L"\\doc\\api";
        if (::GetFileAttributesW(header.c_str()) != INVALID_FILE_ATTRIBUTES &&
            ::GetFileAttributesW(docs.c_str()) != INVALID_FILE_ATTRIBUTES) {
            return dir;
        }
        const std::wstring::size_type pos = dir.find_last_of(L"\\/");
        if (pos == std::wstring::npos) {
            break;
        }
        dir.erase(pos);
    }
    return std::wstring();
}

bool ReadFile(const std::wstring &path, std::string *out) {
    std::ifstream in(path.c_str(), std::ios::binary);
    if (!in) {
        return false;
    }
    std::ostringstream buffer;
    buffer << in.rdbuf();
    *out = buffer.str();
    return true;
}

// 一行里第一个 '(' 之前的最后一个标识符就是方法名：
//   void Capture(_In_ long x1, ...)      -> Capture
//   AStarFindPath(long x1,               -> AStarFindPath（参数跨行，名字仍在第一行）
//   ~Op();                               -> ~Op
std::string MethodNameBeforeFirstParen(const std::string &line) {
    const std::string::size_type paren = line.find('(');
    if (paren == std::string::npos) {
        return std::string();
    }
    const std::string head = line.substr(0, paren);
    std::string::size_type end = head.find_last_not_of(" \t\r\n");
    if (end == std::string::npos) {
        return std::string();
    }
    std::string::size_type begin = end;
    while (begin > 0) {
        const char ch = head[begin - 1];
        const bool ident = (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
                           (ch >= '0' && ch <= '9') || ch == '_' || ch == '~';
        if (!ident) {
            break;
        }
        --begin;
    }
    return head.substr(begin, end - begin + 1);
}

// 构造/析构、运算符、以及访问子服务的 getter 都不是「接口方法」：
// binding()/image()/input()/memory()/opencv()/runtime()/window()/yolo() 只是把内部服务
// 暴露给 C++ 调用方，文档里不单列。
bool IsNoiseName(const std::string &name) {
    if (name.empty()) {
        return true;
    }
    // 析构 ~Op 和赋值运算符 operator= 都不出现在接口文档里。
    // '=' 这类非字母开头的残留片段一律丢掉（operator= 在取「'(' 前最后一个标识符」时
    // 会退化成 '='）。
    if (name[0] == '~' || name[0] < 'A') {
        return true;
    }
    if (name.compare(0, 8, "operator") == 0) {
        return true;
    }

    static const char *kNoise[] = {
        "Op",     "if",    "for",    "while",   "switch", "return",
        "sizeof", "binding", "image", "input",  "memory", "opencv",
        "runtime", "window", "yolo",
    };
    for (const char *item : kNoise) {
        if (name == item) {
            return true;
        }
    }
    return false;
}

// 只取 class OP_API Op 的花括号范围内的方法；文件里还有 class Image、struct point_t 等。
std::set<std::string> ExtractHeaderMethods(const std::string &header) {
    std::set<std::string> names;

    const std::string marker = "class OP_API Op";
    std::string::size_type cursor = header.find(marker);
    if (cursor == std::string::npos) {
        return names;
    }
    cursor = header.find('{', cursor);
    if (cursor == std::string::npos) {
        return names;
    }
    ++cursor;

    std::string line;
    int depth = 1;
    while (cursor < header.size() && depth > 0) {
        const std::string::size_type eol = header.find('\n', cursor);
        const std::string::size_type stop = (eol == std::string::npos) ? header.size() : eol;
        line.assign(header, cursor, stop - cursor);
        cursor = (eol == std::string::npos) ? header.size() : eol + 1;

        // 去掉行尾注释，避免把注释里的括号当成签名。
        const std::string::size_type comment = line.find("//");
        if (comment != std::string::npos) {
            line.erase(comment);
        }

        for (const char ch : line) {
            if (ch == '{') {
                ++depth;
            } else if (ch == '}') {
                --depth;
            }
        }
        if (depth <= 0) {
            break;
        }
        // 顶层深度才是类成员声明；内联函数体里出现的方法调用不算。
        if (depth != 1) {
            continue;
        }
        const std::string name = MethodNameBeforeFirstParen(line);
        if (!IsNoiseName(name)) {
            names.insert(name);
        }
    }
    return names;
}

// 文档里每条方法都写成：- **`void Capture(...)`** → `void`
std::set<std::string> ExtractDocumentedMethods(const std::string &doc) {
    std::set<std::string> names;
    std::istringstream stream(doc);
    std::string line;
    while (std::getline(stream, line)) {
        const std::string::size_type bullet = line.find("- **`");
        if (bullet == std::string::npos) {
            continue;
        }
        const std::string::size_type body = bullet + 5;
        const std::string::size_type close = line.find("`**", body);
        if (close == std::string::npos) {
            continue;
        }
        const std::string name = MethodNameBeforeFirstParen(line.substr(body, close - body));
        if (!IsNoiseName(name)) {
            names.insert(name);
        }
    }
    return names;
}

std::vector<std::wstring> ListDocFiles(const std::wstring &dir) {
    std::vector<std::wstring> paths;
    WIN32_FIND_DATAW data = {0};
    const std::wstring pattern = dir + L"\\*.md";
    HANDLE find = ::FindFirstFileW(pattern.c_str(), &data);
    if (find == INVALID_HANDLE_VALUE) {
        return paths;
    }
    do {
        if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
            paths.push_back(dir + L"\\" + data.cFileName);
        }
    } while (::FindNextFileW(find, &data));
    ::FindClose(find);
    std::sort(paths.begin(), paths.end());
    return paths;
}

std::string JoinMissing(const std::set<std::string> &names) {
    std::string joined;
    for (const std::string &name : names) {
        if (!joined.empty()) {
            joined += ", ";
        }
        joined += name;
    }
    return joined;
}

std::set<std::string> Difference(const std::set<std::string> &left,
                                 const std::set<std::string> &right) {
    std::set<std::string> diff;
    for (const std::string &item : left) {
        if (right.find(item) == right.end()) {
            diff.insert(item);
        }
    }
    return diff;
}

}  // namespace

TEST(ApiDocTest, DocumentedMethodsMatchHeader) {
    const std::wstring root = FindRepoRoot();
    if (root.empty()) {
        GTEST_SKIP() << "找不到仓库根（include/libop.h + doc/api），发布包里不带文档，跳过";
    }

    std::string header;
    ASSERT_TRUE(ReadFile(root + L"\\include\\libop.h", &header))
        << "读不到 include/libop.h";

    const std::vector<std::wstring> doc_files = ListDocFiles(root + L"\\doc\\api");
    ASSERT_FALSE(doc_files.empty()) << "doc/api 下面一个 .md 都没有";

    std::set<std::string> documented;
    for (const std::wstring &path : doc_files) {
        std::string body;
        ASSERT_TRUE(ReadFile(path, &body)) << "读不到文档文件";
        const std::set<std::string> one = ExtractDocumentedMethods(body);
        documented.insert(one.begin(), one.end());
    }

    const std::set<std::string> declared = ExtractHeaderMethods(header);
    ASSERT_FALSE(declared.empty()) << "头文件里一个方法都没解析出来，解析逻辑坏了";

    // 头文件加了方法却没重新生成文档。
    const std::set<std::string> undocumented = Difference(declared, documented);
    EXPECT_TRUE(undocumented.empty())
        << "头文件里有 " << undocumented.size() << " 个方法没进文档："
        << JoinMissing(undocumented)
        << "\n修法：python scripts/gen_api_doc.py";

    // 文档里写了头文件里不存在的方法（改名后忘了同步）。
    const std::set<std::string> phantom = Difference(documented, declared);
    EXPECT_TRUE(phantom.empty())
        << "文档里有 " << phantom.size() << " 个方法在头文件里不存在："
        << JoinMissing(phantom)
        << "\n修法：python scripts/gen_api_doc.py";

    // 两边数量一致才说明没有静默漏项。
    EXPECT_EQ(declared.size(), documented.size())
        << "头文件 " << declared.size() << " 个方法，文档 " << documented.size() << " 个";
}
