"""把 OpAutomation.cpp 里尚未 try/catch 的 COM 方法逐个包起来。

为什么需要：C++ 异常穿过 COM 方法边界（客户端没有 SEH/C++ 处理器）会直接终止进程，
而 183 个 STDMETHODIMP 里只有 8 个手写过 try/catch。这里用脚本做机械包裹，
避免手改 175 个方法时漏改或把括号改坏。

用法：
    python scripts/_wrap_com_trycatch.py            # dry-run，只统计
    python scripts/_wrap_com_trycatch.py --apply    # 写入文件
"""

import re
import sys
from pathlib import Path

TARGET = Path(__file__).resolve().parent.parent / "libop" / "com" / "OpAutomation.cpp"
METHOD_RE = re.compile(r"^STDMETHODIMP\s+OpAutomation::(\w+)\s*\(", re.MULTILINE)

TRY_OPEN = "\n    try {"

# 标量出参在异常路径上必须清零：调用方（尤其易语言/Python 这类只看 ret 的绑定）拿到
# 上一次留下的值会误判成"调用成功"。BSTR*/VARIANT* 不清零 —— 那是调用方持有的指针，
# 我们没写过就不该动它，否则会把调用方原来的对象弄丢（泄漏或重复释放）。
SCALAR_OUT_RE = re.compile(r"^\s*(?:LONG|LONGLONG|ULONG|DOUBLE)\s*\*\s*(\w+)\s*$")


def parse_scalar_outs(text: str, method_end: int) -> list[str]:
    """从方法签名里取出标量出参名（按出现顺序，去重）。"""
    close = text.find(")", method_end)
    if close < 0:
        return []
    # METHOD_RE 停在左括号之后，所以进入时深度已经是 1（人在括号里）。
    depth = 1
    i = method_end
    while i < len(text):
        if text[i] == "(":
            depth += 1
        elif text[i] == ")":
            depth -= 1
            if depth == 0:
                close = i
                break
        i += 1
    args = text[method_end + 1 : close]
    names: list[str] = []
    for raw in args.split(","):
        m = SCALAR_OUT_RE.match(raw)
        if m and m.group(1) not in names:
            names.append(m.group(1))
    return names


def catch_block(scalar_outs: list[str]) -> str:
    body = "".join(f"\n        if ({name}) {{\n            *{name} = 0;\n        }}" for name in scalar_outs)
    return f"\n    }} catch (...) {{{body}\n        return E_UNEXPECTED;\n    }}\n"


def skip_literal_or_comment(text: str, i: int) -> int:
    """若 text[i] 是注释或字面量的起始，返回其结束位置；否则返回 -1。"""
    if text.startswith("//", i):
        j = text.find("\n", i)
        return len(text) if j < 0 else j
    if text.startswith("/*", i):
        j = text.find("*/", i + 2)
        return len(text) if j < 0 else j + 2
    ch = text[i]
    if ch in "\"'":
        j = i + 1
        while j < len(text):
            if text[j] == "\\":
                j += 2
                continue
            if text[j] == ch:
                return j + 1
            j += 1
        return len(text)
    return -1


def find_matching_brace(text: str, open_idx: int) -> int:
    depth = 0
    i = open_idx
    while i < len(text):
        skip = skip_literal_or_comment(text, i)
        if skip >= 0:
            i = skip
            continue
        if text[i] == "{":
            depth += 1
        elif text[i] == "}":
            depth -= 1
            if depth == 0:
                return i
        i += 1
    return -1


def find_body_brace(text: str, method_start: int) -> int:
    i = method_start
    while i < len(text):
        skip = skip_literal_or_comment(text, i)
        if skip >= 0:
            i = skip
            continue
        if text[i] == "{":
            return i
        i += 1
    return -1


def main() -> int:
    apply = "--apply" in sys.argv
    text = TARGET.read_text(encoding="utf-8")

    edits = []  # (insert_pos, text)
    wrapped, skipped, broken = [], [], []
    zeroed: list[tuple[str, list[str]]] = []
    for m in METHOD_RE.finditer(text):
        name = m.group(1)
        open_idx = find_body_brace(text, m.end())
        if open_idx < 0:
            broken.append(name)
            continue
        close_idx = find_matching_brace(text, open_idx)
        if close_idx < 0:
            broken.append(name)
            continue
        body = text[open_idx + 1 : close_idx]
        if re.search(r"\btry\b", body):
            skipped.append(name)
            continue
        edits.append((open_idx + 1, TRY_OPEN))
        outs = parse_scalar_outs(text, m.end())
        edits.append((close_idx, catch_block(outs)))
        wrapped.append(name)
        if outs:
            zeroed.append((name, outs))

    print(f"方法总数        : {len(skipped) + len(wrapped) + len(broken)}")
    print(f"已有 try/catch  : {len(skipped)}")
    print(f"本次将包裹      : {len(wrapped)}")
    if broken:
        print(f"括号匹配失败    : {broken}")
        return 1
    print(f"插入点          : {len(edits)}（每方法 2 处）")
    print("样例            : " + ", ".join(wrapped[:8]))
    print(f"带标量出参清零  : {len(zeroed)}（例：{zeroed[0] if zeroed else None}）")

    if not apply:
        print("\n[dry-run] 未写入。加 --apply 生效。")
        return 0

    for pos, snippet in sorted(edits, key=lambda item: item[0], reverse=True):
        text = text[:pos] + snippet + text[pos:]
    TARGET.write_text(text, encoding="utf-8")
    print(f"\n已写入 {TARGET}（{len(wrapped)} 个方法包裹完成）")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

