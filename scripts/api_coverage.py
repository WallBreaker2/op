#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""精确统计三层公开 API 的测试覆盖缺口（只读）。

v2 相比 v1：
- 大小写不敏感匹配（UnBindWindow vs UnbindWindow 这类不再算缺口）
- 排除子服务访问器（binding/image/input/memory/opencv/runtime/window/yolo）
- 结果同时写一份 _backup/api_coverage_report.txt，方便贴进文档

用法：python _backup/api_coverage.py
"""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

KEYWORDS = {
    'if', 'for', 'while', 'switch', 'return', 'sizeof', 'catch', 'do',
    'else', 'case', 'new', 'delete', 'throw', 'static_cast', 'reinterpret_cast',
    'const_cast', 'dynamic_cast', 'decltype', 'alignof', 'typeid', 'operator',
}

# 子服务访问器：返回对象引用，不是可调用 API
ACCESSORS = {
    'binding', 'image', 'input', 'memory', 'opencv', 'runtime', 'window', 'yolo',
    'com', 'hook', 'capture', 'network', 'ocr',
}


def read(path):
    with open(path, 'r', encoding='utf-8', errors='ignore') as f:
        return f.read()


def op_class_methods():
    text = read(os.path.join(ROOT, 'include', 'libop.h'))
    start = text.find('class OP_API Op')
    if start < 0:
        start = text.find('class Op')
    end = text.find('\n};', start) if start >= 0 else -1
    body = text[start:end if end > 0 else len(text)]
    names = set()
    for m in re.finditer(r'^\s{4,}(?:OP_API\s+)?[A-Za-z_][\w:<>,\s\*&]*?\b([A-Za-z_]\w*)\s*\(', body, re.M):
        n = m.group(1)
        if n in KEYWORDS or n in ACCESSORS:
            continue
        names.add(n)
    return sorted(names)


def c_api_exports():
    text = read(os.path.join(ROOT, 'libop', 'c_api', 'op_c_api.cpp'))
    names = set()
    for m in re.finditer(r'OP_CALL\s+Op([A-Z]\w*)\s*\(', text):
        n = m.group(1)
        if n.lower() in ACCESSORS:
            continue
        names.add(n)
    return sorted(names)


def com_dispids():
    path = os.path.join(ROOT, 'libop', 'com', 'op.idl')
    if not os.path.exists(path):
        return []
    text = read(path)
    # 跳过整行注释：op.idl 里有一段 // [id(N)] HRESULT ... 的遗留清单，
    # 它们不是活动接口，算进缺口会是假阳性。
    text = '\n'.join(line for line in text.splitlines() if not line.lstrip().startswith('//'))
    names = set()
    for m in re.finditer(r'HRESULT\s+(\w+)\s*\(', text):
        n = m.group(1)
        if n in ('QueryInterface', 'AddRef', 'Release', 'GetTypeInfoCount',
                 'GetTypeInfo', 'GetIDsOfNames', 'Invoke'):
            continue
        names.add(n)
    return sorted(names)


def test_corpus():
    chunks = []
    tdir = os.path.join(ROOT, 'tests')
    for name in sorted(os.listdir(tdir)):
        if name.endswith(('.cpp', '.h')):
            chunks.append(read(os.path.join(tdir, name)))
    return '\n'.join(chunks)


def called_names(corpus):
    names = set()
    for m in re.finditer(r'(?:\.|->|::)\s*([A-Za-z_]\w*)\s*\(', corpus):
        names.add(m.group(1).lower())
    for m in re.finditer(r'\bOp([A-Z]\w*)\s*\(', corpus):
        names.add(m.group(1).lower())
    return names


OUT = []


def emit(line=''):
    print(line)
    OUT.append(line)


def report(title, universe, called):
    missing = [n for n in universe if n.lower() not in called]
    covered = len(universe) - len(missing)
    pct = (100.0 * covered / len(universe)) if universe else 0.0
    emit('=' * 74)
    emit('%s' % title)
    emit('共 %d，被测试调用 %d（%.1f%%），未覆盖 %d'
         % (len(universe), covered, pct, len(missing)))
    emit('=' * 74)
    if missing:
        for i in range(0, len(missing), 5):
            emit('  ' + '  '.join('%-24s' % n for n in missing[i:i + 5]))
    emit()
    return missing


def main():
    corpus = test_corpus()
    called = called_names(corpus)

    ops = op_class_methods()
    capi = c_api_exports()
    com = com_dispids()

    emit('测试源码语料：%d 字符' % len(corpus))
    emit()
    m1 = report('C++ 公开 API（include/libop.h 的 class Op）', ops, called)
    m2 = report('C API（libop/c_api/op_c_api.cpp 的 OP_C_API 导出）', capi, called)
    m3 = report('COM 接口（libop/com/op.idl）', com, called)

    emit('-' * 74)
    emit('三层共同的缺口（C++ 与 C API 都没测到）：')
    common = sorted(set(m1) & set(m2), key=str.lower)
    for i in range(0, len(common), 5):
        emit('  ' + '  '.join('%-24s' % n for n in common[i:i + 5]))
    emit()
    emit('合计缺口：C++ %d 个，C API %d 个，COM %d 个' % (len(m1), len(m2), len(m3)))

    with open(os.path.join(ROOT, '_backup', 'api_coverage_report.txt'), 'w',
              encoding='utf-8') as f:
        f.write('\n'.join(OUT) + '\n')
    return 0


if __name__ == '__main__':
    sys.exit(main())
