"""从 include/libop.h 生成 doc/api.md（索引）与 doc/api/*.md（分主题的函数文档）。

用法：
    python scripts/gen_api_doc.py

头文件里每个公开方法上面都有一行 `//` 注释，本脚本把它和方法签名配对输出。
改完头文件重跑一次即可，文档不会和代码脱节。
"""

import io
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "include", "libop.h")
DST = os.path.join(ROOT, "doc", "api.md")

SECTION_RE = re.compile(r"^\s*//\s*[-\u2014=]{3,}\s*(.+?)\s*[-\u2014=]{3,}\s*$")
METHOD_RE = re.compile(
    r"^\s{4,}([\w:<>,\*&\s\[\]]+?)\s+(\w+)\s*\((.*)\)\s*(?:const)?\s*;\s*(?://(.*))?$"
)

# 头文件里没写注释的接口，在这里补。键是函数名，值是文档里显示的那句说明。
# 这些说明来自实现（libop/op/OpMemory.cpp、libop/op/OpOpenCv.cpp、libop/op/OpRuntime.cpp 等），
# 不是从名字反推的。
EXTRA_DOC = {
    # ---- 基本设置 / 截图 ----
    "GetLastError": "取最近一次内部错误的描述文本。与 Win32 的 `::GetLastError` 无关："
                    "两者同名，头文件里已 `#undef` 相关宏；本方法返回的是插件自己的错误原因。",
    "KeyPressChar": "按字符码按下并释放一个键。`vk_code` 传虚拟键码的十进制字符串（如 `L\"65\"` 即 `A`）；"
                    "要输入整串文本用 `SendString`。",
    "LoadPic": "把图片文件预加载进插件内部图片缓存，后续 `FindPic` 按名字命中缓存、不再读盘。",
    "FreePic": "释放 `LoadPic` 预加载的缓存项。不调用也不影响正确性，只是占内存。",
    "GetScreenData": "取指定区域的**原始像素**（BGRA 紧凑排列），不是 BMP。"
                     "出参 `data` 指向内部复用缓冲，**下一次调用同一函数就会被覆盖**。",
    "GetScreenDataBmp": "取指定区域的画面并编码成 24 位 BMP 字节流。出参 `data` 同样指向内部复用缓冲。",
    "GetScreenFrameInfo": "取当前帧的序号与时间戳（毫秒）。用于判断“画面有没有变”，比逐像素比较便宜。",
    "MatchPicName": "按 BMP 命名规则补全图片名（相对 `SetPath` 设置的目录），返回实际要打开的文件名。",

    # ---- OpenCV：模板管理 ----
    "CvLoadTemplate": "按名字加载一张模板图，之后 `CvMatch*` 用名字引用它。",
    "CvLoadMaskedTemplate": "加载带掩模的模板图：只有掩模非零的像素参与匹配，用于目标带透明背景或杂边的场景。",
    "CvRemoveTemplate": "移除指定名字的模板。",
    "CvRemoveAllTemplates": "清空所有已加载的模板。",
    "CvHasTemplate": "查询某个名字的模板是否已加载（`ret` 为 1 表示存在）。",
    "CvGetTemplateCount": "取当前已加载的模板数量。",
    "CvGetAllTemplateNames": "取全部模板名，用 `|` 连接。",
    "CvGetOpenCvVersion": "取底层 OpenCV 的版本字符串。",
    "CvLoadTemplateList": "批量加载模板。清单格式是 `名字,路径|名字,路径`（逗号分隔名与路径，竖线分隔条目）。",

    # ---- OpenCV：图像文件处理（输入输出都是磁盘上的 BMP/PNG） ----
    "CvToGray": "转灰度图。",
    "CvToBinary": "按内部默认参数二值化（需要自定义阈值时用 `CvThreshold`）。",
    "CvToEdge": "提取边缘（Canny）。",
    "CvToOutline": "提取轮廓线。",
    "CvDenoise": "去噪（中值/高斯滤波的组合）。",
    "CvEqualize": "直方图均衡化，提升低对比度图的可辨识度。",
    "CvCLAHE": "自适应直方图均衡（CLAHE）。`clip_limit` 控对比度上限，`tile_grid_size` 是分块大小。",
    "CvBlur": "模糊。`mode` 选模糊类型，`kernel_size` 是核尺寸（必须为正奇数）。",
    "CvSharpen": "锐化，`strength` 控制强度。",
    "CvCropValid": "自动裁掉四周的纯色边框，只留有内容的部分。",
    "CvConnectedComponents": "连通域统计，出参是 JSON：每个连通域的位置、面积；`min_area` 以下的会被丢掉。",
    "CvFindContours": "查找轮廓，出参 JSON 里每个轮廓带外接矩形与面积；`min_area` 以下丢弃。",
    "CvPreprocessPipeline": "按给定步骤串联做图像预处理。`pipeline` 形如 `gray|threshold,120|morphology,open`，"
                             "每步只把当前图像交给下一步。",
    "CvCrop": "按矩形裁剪。",
    "CvResize": "缩放到指定宽高。",
    "CvThreshold": "二值化。`threshold` 是阈值、`max_value` 是超过阈值时的赋值，`type` 对应 OpenCV 的 `THRESH_*`。",
    "CvInRange": "按颜色区间做掩模。`color_space` 选色彩空间（如 `hsv`/`bgr`），上下界是 `min|max` 形式的字符串。",
    "CvMorphology": "形态学运算。`mode` 取 `erode`/`dilate`/`open`/`close`，`kernel_size` 是结构元尺寸。",
    "CvThin": "细化（骨架化），`mode` 选择细化算法。",

    # ---- OpenCV：在当前捕获图上匹配 ----
    # 这一组的 x/y/width/height 是“当前捕获图上的矩形”，不是屏幕坐标也不是窗口坐标。
    "CvMatchTemplate": "在**当前捕获图**的 `x,y,width,height` 区域内匹配单模板。出参 JSON：`{\"ok\":1,\"x\":..,\"y\":..,"
                       "\"width\":..,\"height\":..,\"score\":..}`，失败是 `{\"ok\":0}`。",
    "CvMatchTemplateScale": "同上，但会按 `scales`（如 `1.0|0.9|1.1`）逐档缩放模板后再匹配，用于目标大小不确定的场景。",
    "CvMatchAnyTemplate": "在多个模板里取最先命中的那个。`template_names` 用 `|` 分隔。",
    "CvMatchAllTemplates": "把区域内所有模板的所有命中位置都返回，出参是多条 JSON。",
    "CvFeatureMatchTemplate": "特征点匹配（对缩放、旋转、光照变化更稳，但比模板匹配慢）。出参同样是 JSON。",
    "CvEdgeMatchTemplate": "基于边缘的模板匹配，适合背景杂乱但目标轮廓清晰的场景。",
    "CvShapeMatchTemplate": "基于形状的匹配，适合目标内部纹理变化大、只有外形稳定的场景。",

    # ---- OCR / YOLO ----
    "SetOcrEngine": "选择 OCR 引擎（本地字典 or HTTP 服务）。不设置时用默认策略：有字典走本地，否则走 HTTP。",
    "SetYoloEngine": "设置 YOLO 检测服务的端点，形如 `http://127.0.0.1:8090/api/v1/detect`。",
    "YoloDetect": "在**当前捕获图**的 `x1,y1,x2,y2` 区域内做目标检测，`conf` 是置信度门限、`iou` 是 NMS 的 IoU 门限，"
                  "出参是 JSON。",
    "YoloDetectFromFile": "对指定图片文件做目标检测，参数与出参同 `YoloDetect`。",

    # ---- 内存读写 ----
    # hwnd 传 0 表示“当前绑定窗口”；本进程走 memcpy，跨进程走注入读取。
    "WriteInt": "写整数。`type` 决定宽度与符号：`0`/`4` → 4 字节、`1`/`5` → 2 字节、`2`/`6` → 1 字节、`3` → 8 字节。",
    "WriteIntBatch": "批量写整数。`addresses` 与 `values` 等长，`ok` 逐项标记成功与否——**单项失败不影响其它项**。",
    "ReadFloat": "读一个 32 位浮点数。",
    "WriteFloat": "写一个 32 位浮点数。",
    "ReadDouble": "读一个 64 位浮点数。",
    "WriteDouble": "写一个 64 位浮点数。",
    "ReadString": "读字符串。`type` 是历史 DM 约定：`0` = ACP/GBK、`1` = UTF-16、`2` = UTF-8；"
                  "`len` 是**字节数**（UTF-16 时每字符 2 字节）。",
    "WriteString": "写字符串，`type` 与 `len` 语义同 `ReadString`。",
}

# 子服务访问器与构造/赋值不是“函数调用文档”的内容
SKIP_NAMES = {
    "Op", "operator=", "runtime", "window", "binding", "input", "image",
    "ocr", "opencv", "yolo", "memory",
}

HEADER = """# op::Op 函数调用文档

本文件由 `scripts/gen_api_doc.py` 从 `include/libop.h` 生成，签名与注释永远和代码同源。
改完头文件重跑一次脚本即可。

## 通用约定

### `long *ret` 的含义

绝大多数接口的最后一个参数是 `long *ret`，含义只有一个：**这次调用是否成功**。

| 值 | 含义 |
|---|---|
| `1` | 成功 |
| `0` | 失败（参数非法 / 目标不存在 / 超时 / 后端不支持） |

**它不承载业务数据。** 业务数据走各自的出参（`rect_t &`、`wstring &`、`bytearray &`、`point_t &`）。

```cpp
op::Op op;
long ret = 0;
rect_t rect;
op.FindPic(0, 0, 2000, 2000, L"target.bmp", L"101010", 0.9, 0, &rect, &ret);
if (ret != 1) {
    // 没找到；此时 rect 的内容不保证有意义
}
```

### 坐标与尺寸

- 所有坐标都是**绑定目标窗口的客户区坐标**（`BindWindow` 之后），不是屏幕坐标。
- 需要屏幕坐标时用 `ClientToScreen` / `ScreenToClient` 显式换算。
- `rect_t` 的字段是 `x` / `y` / `width` / `height`（`libop/runtime/Types.h`）。

### 错误信息怎么出来

`SetShowErrorMsg(mode)`：`0` 关闭、`1` 信息框、`2` 写文件（`__op.log`）、`3` 标准输出，默认 `1`。
服务端或无桌面程序里，第一件事就该设成 `0` 或 `2`——否则失败时弹框会把调用线程卡住。

### 线程安全

同一个 `op::Op` 实例**不是线程安全的**：OCR 与找图系列共用“当前捕获图” `_src`，并发调用会互相覆盖。
一个线程一个实例，或者自己加锁。

例外：HTTP session 池、截图设备的共享缓存是进程级加锁的，跨实例并发安全。

### 从 C++ 到 C API

`libop/c_api/op_c_api.cpp` 的导出是 C++ 接口的扁平化版本，命名规则是 `Op` + 方法名，
参数顺序不变，引用出参换成指针。

| C++ | C API |
|---|---|
| `op.Version(str)` | `OpVersion(wchar_t **str)` |
| `op.FindPic(x1,y1,x2,y2,name,delta,sim,dir,rect,ret)` | `OpFindPic(hwnd, x1,y1,x2,y2,name,delta,sim,dir,long *x,long *y,long *ret)` |

返回 `wchar_t *` 的接口，指针指向**句柄内部**的环形缓冲池（4 槽轮转）：

- 最近 4 次调用返回的指针**同时有效**，可以放心地拿第一个返回值当第二次调用的入参
  （`OpGetWordResultStr(h, OpFetchWords(...), 0)` 这种链式用法就是这样）。
- 第 5 次调用会回收最早那一槽，指向它的旧指针**立刻失效**，读到的是新内容。
- 要长期保留就自己拷出来（`Marshal.PtrToStringUni` / `std::wstring(...)`）。

例外：这些指针属于**单个句柄**，不同 `op_handle` 之间互不干扰，多线程各持一个句柄是安全的。

这一段约定踩过两次坑：§72.48 修的是入参自别名（`OpGetWordResultStr` 的第一个参数就是上一次
返回值，清理缓冲会把入参抹成空串），§72.58 修的是短字符串（≤7 个 wchar，如 `GetColor` 的
`RRGGBB`）落在 `std::wstring` 的 SSO 里、`swap` 换不掉 buffer 地址，于是「上一轮」那份缓冲
照样被改写 —— 换成槽位池之后才真正成立。

### 典型调用序列

```cpp
op::Op op;
long ret = 0;

op.SetShowErrorMsg(0, &ret);                       // 后台程序：别弹框
op.BindWindow(hwnd, L"normal", L"normal", L"normal", 0, &ret);  // 绑窗口

rect_t found;
op.FindPic(0, 0, 800, 600, L"button.bmp", L"101010", 0.85, 0, &found, &ret);
if (ret == 1) {
    op.MoveTo(found.x + found.width / 2, found.y + found.height / 2, &ret);
    op.LeftClick(&ret);
}

op.UnBindWindow(&ret);
```
"""


def split_sections(lines):
    """按头文件里的 `//----标题----` 注释切分，返回 [{title, items}]。"""
    start = None
    for index, line in enumerate(lines):
        if re.match(r"^\s*class\s+OP_API\s+Op\b", line):
            start = index
            break
    if start is None:
        sys.exit("include/libop.h 里找不到 class OP_API Op")

    end = len(lines)
    for index in range(start + 1, len(lines)):
        if re.match(r"^\s{0,4}\};", lines[index]):
            end = index
            break

    sections = [{"title": "基本设置/属性", "items": []}]
    pending = []
    buf = None  # 跨行签名的累积缓冲

    def flush(declaration, note_text, pending_comments):
        matched = METHOD_RE.match("    " + declaration)
        if not matched:
            return
        head = " ".join(matched.group(1).split())
        name = matched.group(2)
        if name in SKIP_NAMES:
            return
        ret_type = head
        params = " ".join(matched.group(3).split())
        note = (matched.group(4) or "").strip() or note_text
        doc = " ".join(pending_comments).strip()
        if note:
            doc = (doc + " " + note).strip()
        sections[-1]["items"].append({
            "ret": ret_type,
            "name": name,
            "params": params,
            "doc": doc,
        })

    for line in lines[start + 1:end]:
        stripped = line.strip()

        if buf is not None:
            buf += " " + stripped
            if ";" in stripped:
                flush(buf, "", pending)
                buf = None
                pending = []
            continue

        matched = SECTION_RE.match(line)
        if matched:
            title = matched.group(1).strip()
            if title and (not sections[-1]["items"]):
                sections[-1]["title"] = title
            elif title:
                sections.append({"title": title, "items": []})
            pending = []
            continue

        if stripped.startswith("//"):
            text = stripped[2:].strip()
            if text:
                pending.append(text)
            continue

        if not stripped or stripped.startswith("#") or stripped.startswith("using "):
            pending = []
            continue

        if "(" in stripped:
            if ";" in stripped:
                flush(stripped, "", pending)
                pending = []
            else:
                buf = stripped
            continue

        pending = []

    return [s for s in sections if s["items"]]


# 头文件里的 8 个分组按主题归并成 6 个文档文件。
# 每一项是 (文件名, 文件标题, 覆盖的分组标题)。
GROUPS = [
    ("01-runtime", "运行时与算法", ["基本设置/属性", "algorithm"]),
    ("02-window", "窗口与进程", ["windows api", "Background"]),
    ("03-input", "鼠标与键盘", ["mouse & keyboard"]),
    ("04-image", "图像与颜色", ["image and color"]),
    ("05-opencv", "OpenCV", ["opcv"]),
    ("06-ocr", "OCR 与 YOLO", ["ocr"]),
]
FALLBACK_FILE = ("07-other", "其他")


def render_items(out, section):
    for item in section["items"]:
        signature = "%s %s(%s)" % (item["ret"], item["name"], item["params"])
        out.append("- **`%s`** → `%s`" % (signature, item["ret"]))
        doc = item["doc"] or EXTRA_DOC.get(item["name"], "")
        if doc:
            out.append("  %s" % doc)
        else:
            out.append("  *（头文件里没写注释，参数含义看签名里的 SAL 标注）*")
        out.append("")


def build_files(sections):
    """把分组归拢成文件，返回 [(文件名, 文件标题, [分组])]。

    头文件里新增了 GROUPS 没覆盖到的分组时，落到 FALLBACK_FILE，不会漏掉任何方法。
    """
    by_title = dict((section["title"], section) for section in sections)
    used = set()
    files = []
    for name, title, group_titles in GROUPS:
        picked = [by_title[item] for item in group_titles if item in by_title]
        if not picked:
            continue
        used.update(item for item in group_titles if item in by_title)
        files.append((name, title, picked))
    rest = [section for section in sections if section["title"] not in used]
    if rest:
        files.append((FALLBACK_FILE[0], FALLBACK_FILE[1], rest))
    return files


def render_index(files, total):
    out = [HEADER, "## 接口索引", ""]
    out.append("| 文档 | 覆盖的分组 | 函数数 |")
    out.append("|---|---|---|")
    for name, title, picked in files:
        count = sum(len(section["items"]) for section in picked)
        groups = "\u3001".join(section["title"] for section in picked)
        out.append("| [%s](api/%s.md) | %s | %d |" % (title, name, groups, count))
    out.append("| **合计** | | **%d** |" % total)
    out.append("")
    out.append("每个方法都带签名和一句说明；说明来自头文件注释，"
               "头文件没写注释的由 `scripts/gen_api_doc.py` 的 `EXTRA_DOC` 对着实现补。")
    out.append("")
    out.append("## 按任务查")
    out.append("")
    out.append("[常见任务配方](api/recipes.md)：截图存盘、找图后点击、OCR 取文字、"
               "跨进程读内存、多窗口批量绑定、窗口位置整理。")
    out.append("每个配方都是完整可编译的一段代码，外加这类任务最容易踩的坑。")
    out.append("")
    out.append("这个文件是手写的，生成器不会覆盖它。")
    out.append("")
    return "\n".join(out) + "\n"


def render_file(name, title, picked):
    count = sum(len(section["items"]) for section in picked)
    out = ["# %s" % title, ""]
    out.append("`op::Op` 的这一组接口共 **%d** 个。返回索引：[doc/api.md](../api.md)" % count)
    out.append("")
    out.append("> 通用约定（`long *ret` 的含义、坐标语义、线程安全、C API 命名映射）"
               "见 [索引页](../api.md#通用约定)。")
    out.append("")
    for section in picked:
        out.append("## %s（%d）" % (section["title"], len(section["items"])))
        out.append("")
        render_items(out, section)
    return "\n".join(out) + "\n"


def main():
    lines = io.open(SRC, encoding="utf-8", errors="replace").read().splitlines()
    sections = split_sections(lines)
    files = build_files(sections)
    total = sum(len(section["items"]) for section in sections)

    out_dir = os.path.join(ROOT, "doc", "api")
    if not os.path.isdir(out_dir):
        os.makedirs(out_dir)

    io.open(DST, "w", encoding="utf-8", newline="\n").write(render_index(files, total))
    print("index -> %s" % DST)
    for name, title, picked in files:
        path = os.path.join(out_dir, name + ".md")
        io.open(path, "w", encoding="utf-8", newline="\n").write(
            render_file(name, title, picked))
        count = sum(len(section["items"]) for section in picked)
        print("  %-22s %-12s %d" % (name + ".md", title, count))
    print("sections=%d methods=%d files=%d" % (len(sections), total, len(files)))


if __name__ == "__main__":
    main()
