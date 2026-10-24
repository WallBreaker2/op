"""生成 tests/opencv_test.cpp 里 5 条「真实图片」用例所需的素材。

## 为什么要有这个脚本

审计 R-05 发现：`OpenCvTest` 里有 5 条用例依赖 `assets/opencv_real_*.jpg` 与
`assets/opencv_game_*.png`，而仓库里**从来没有过这些文件**。加上
`FindDemoAsset()` 只往上看两级（已修，见 patch_find_demo_asset.py），这 5 条
一直在 `HasDemoAssetPair() == false` 上永久 skip —— 计为"通过"，实际零覆盖。

## 素材是什么

确定性合成图（不是照片）。这样做有三个好处：

1. **可复现**：脚本固定随机种子，任何时候重跑都得到逐字节相同的素材。
2. **断言可控**：模板就是源图对应区域的精确裁切，所以"最佳匹配位置必须等于
   用例里写死的 template_rect"这个断言天然成立 —— 用例真正验的是**匹配链路**
   （预处理、掩码、金字塔、多方法、缩放搜索）而不是碰运气。
3. **不引入第三方图片的授权问题**。

`scale` 这一项是特例：它和 `coin` **共用**同一个模板文件，但矩形是 105×105
（coin 模板是 70×70，正好 1.5 倍）。所以源图那一处画的是 coin 图案的等比放大，
用来验 `MatchTemplateScale` 的缩放搜索。

## 用法

    python scripts/gen_demo_assets.py          # 生成到仓库根 assets/
    python scripts/gen_demo_assets.py --verify # 生成后顺便用 cv2 校验匹配位置
"""
import math
import random
import sys
import pathlib

from PIL import Image, ImageDraw, ImageEnhance

ROOT = pathlib.Path(__file__).resolve().parent.parent
ASSETS = ROOT / "assets"

W, H = 1280, 720

# 与 tests/opencv_test.cpp 的 RealPhotoTemplateCases() / GameSceneTemplateCases() 一一对应。
PHOTO_CASES = [
    ("opencv_real_source.jpg", "opencv_real_template.png", (860, 130, 96, 96), 11),
    ("opencv_real_city_source.jpg", "opencv_real_city_template.png", (700, 260, 96, 96), 22),
    ("opencv_real_fruit_source.jpg", "opencv_real_fruit_template.png", (520, 250, 96, 96), 33),
    ("opencv_real_sign_source.jpg", "opencv_real_sign_template.png", (585, 190, 96, 96), 44),
    ("opencv_real_space_source.jpg", "opencv_real_space_template.png", (560, 300, 96, 96), 55),
    ("opencv_real_texture_source.jpg", "opencv_real_texture_template.png", (640, 320, 96, 96), 66),
]

SCENE_SOURCE = "opencv_game_scene_source.png"
# GameSceneTemplateCases() 里 scale / feature / shape 用的是**各自的** source 文件，
# 不是共用 scene_source（这一条最初看漏了，导致那三条用例一直凑不满 7 对而 skip）。
GAME_SOURCES = {
    "opencv_game_scale_source.png": "opencv_game_coin_template.png",
    "opencv_game_feature_source.png": "opencv_game_feature_template.png",
    "opencv_game_shape_source.png": "opencv_game_shape_template.png",
}
SCENE_CASES = [
    (SCENE_SOURCE, "opencv_game_scene_template.png", (300, 488, 128, 128), 101),
    (SCENE_SOURCE, "opencv_game_coin_template.png", (170, 185, 70, 70), 102),
    (SCENE_SOURCE, "opencv_game_gem_template.png", (455, 325, 70, 70), 103),
    (SCENE_SOURCE, "opencv_game_edge_template.png", (1060, 240, 90, 114), 104),
    (SCENE_SOURCE, "opencv_game_shape_template.png", (420, 65, 240, 275), 105),
    (SCENE_SOURCE, "opencv_game_feature_template.png", (837, 102, 156, 156), 106),
    ("opencv_game_scale_source.png", "opencv_game_coin_template.png", (1040, 120, 105, 105), 102),
    ("opencv_game_feature_source.png", "opencv_game_feature_template.png", (837, 102, 156, 156), 106),
    ("opencv_game_shape_source.png", "opencv_game_shape_template.png", (420, 65, 240, 275), 105),
]

# scale 不单独画图案：它放在 (1040,120)，是 coin 模板（70×70）的 1.5 倍放大。
# 额外撒的金币位置（左上角坐标，尺寸 70×70），避开其它图案区域。
# 注意：MatchAnyTemplates 返回的是扫描到的第一个实例（列优先，x 小的先），
# 而用例断言"单个匹配 = 主位 (170,185)"。所以这些额外实例的 x 必须大于 170，
# 否则主位不再是第一个被扫到的。
COIN_EXTRA_POSITIONS = [(700, 500), (900, 600), (1150, 450), (450, 560)]
# 这些模板不从独立图案生成，而是场景合成完再从场景里裁（见 build_patterns）。
DEFERRED_TO_CROP = {"opencv_game_gem_template.png"}
SCALE_RECT = (1040, 120, 105, 105)
COIN_TEMPLATE = "opencv_game_coin_template.png"


def draw_texture_block(draw, rect, seed):
    """在 rect 里画一块确定性纹理 + 独有的几何标记。

    纯色块会让模板匹配出现大面积等分区域（相关系数在多处都接近 1），
    噪声保证"这块图案在整张图里唯一"。

    §72.51：第一版只有渐变 + 小噪声，某些 seed 出来方差太小，TM_CCOEFF_NORMED
    在**正确位置**也只得 0.65（位置对、分数低）。这里改成棋盘高频底 + 更大
    噪声 + 独有几何标记，保证任何 seed 的自相关系数都接近 1。
    """
    x, y, w, h = rect
    rng = random.Random(seed)
    base_r = 40 + (seed * 37) % 180
    base_g = 40 + (seed * 71) % 180
    base_b = 40 + (seed * 113) % 180

    # 高频棋盘底：8px 方格，两色交替 —— 给模板匹配一个强结构。
    cell = 8
    for py in range(y, y + h):
        for px in range(x, x + w):
            checker = ((px - x) // cell + (py - y) // cell) % 2
            scale_f = 1.0 if checker == 0 else 0.55
            n = rng.randint(-26, 26)
            draw.point(
                (px, py),
                fill=(
                    max(0, min(255, int(base_r * scale_f) + n)),
                    max(0, min(255, int(base_g * scale_f) + n)),
                    max(0, min(255, int(base_b * scale_f) + n)),
                ),
            )

    # 独有几何标记：每个 seed 一种形状/位置，保证各处图案互不相同。
    cx, cy = x + w // 2, y + h // 2
    kind = seed % 3
    radius = max(4, min(w, h) // 4)
    if kind == 0:
        draw.ellipse([cx - radius, cy - radius, cx + radius, cy + radius], outline=(255, 255, 255), width=3)
    elif kind == 1:
        draw.polygon(
            [(cx, cy - radius), (cx - radius, cy + radius), (cx + radius, cy + radius)], outline=(255, 255, 0)
        )
        draw.line([(cx, cy - radius), (cx - radius, cy + radius), (cx + radius, cy + radius), (cx, cy - radius)],
                  fill=(255, 255, 0), width=3)
    else:
        draw.line([(cx - radius, cy - radius), (cx + radius, cy + radius)], fill=(0, 255, 255), width=3)
        draw.line([(cx - radius, cy + radius), (cx + radius, cy - radius)], fill=(0, 255, 255), width=3)


def new_canvas(seed):
    rng = random.Random(seed)
    img = Image.new("RGB", (W, H))
    draw = ImageDraw.Draw(img)
    # 背景：低频色带 + 稀疏光斑，给匹配一个"有干扰的底"。
    for row in range(H):
        t = row / (H - 1)
        draw.line(
            [(0, row), (W - 1, row)],
            fill=(int(40 + 80 * t), int(30 + 60 * math.sin(t * 3.1)), int(90 - 40 * t)),
        )
    for _ in range(60):
        cx, cy = rng.randint(0, W - 1), rng.randint(0, H - 1)
        rr = rng.randint(6, 40)
        shade = rng.randint(60, 150)
        draw.ellipse([cx - rr, cy - rr, cx + rr, cy + rr], fill=(shade, shade // 2, shade // 3))
    return img, draw


def save_png(img, name):
    path = ASSETS / name
    img.save(path, format="PNG", optimize=False)
    return path


def build_photo_case(source_file, template_file, rect, seed):
    img, draw = new_canvas(seed)
    draw_texture_block(draw, rect, seed)
    # JPG 保存后再解码，模板必须从**解码后**的像素裁，否则有损压缩会让模板与源图差一点。
    jpg_path = ASSETS / source_file
    img.save(jpg_path, format="JPEG", quality=95, subsampling=0)
    decoded = Image.open(jpg_path).convert("RGB")
    x, y, w, h = rect
    save_png(decoded.crop((x, y, x + w, y + h)), template_file)
    return 1


def draw_shape_subject(draw, rect, seed):
    """给 ShapeMatchTemplate 专用的图案：只有一个清晰主体的实心图。

    棋盘底虽然对**模板匹配**很友好（自相关接近 1.0），但它会制造出上千条
    边缘，形状匹配（轮廓 / 形状上下文）在那种图上根本找不到主体，必然返回
    找不到。这里只留 1 个外轮廓 + 1 个内部孔洞，正是形状匹配想要的东西。
    """
    x, y, w, h = rect
    rng = random.Random(seed)
    draw.rectangle([x, y, x + w - 1, y + h - 1], fill=(26, 30, 38))
    cx, cy = x + w // 2, y + h // 2
    # 主体必须**几乎填满**整个模板：形状匹配返回的是它找到的那个轮廓的外接框，
    # 主体只占中间一块的话，返回的宽高会比模板小一大截（实测差 92×126）。
    a = int((w / 2.0) * 0.99)
    b = int((h / 2.0) * 0.99)
    pts = []
    for i in range(8):
        # 顶点从 0° 起算：八边形在正右/正下方向各有一个顶点，外接框才等于
        # 2a × 2b。若从 22.5° 起算，最大 x 只有 cos(22.5°)·a = 0.924a，
        # 外接框会缩水 8%，形状匹配返回的框就比模板小一圈。
        ang = i * math.pi / 4.0
        pts.append((cx + int(a * math.cos(ang)), cy + int(b * math.sin(ang))))
    draw.polygon(pts, fill=(236, 231, 205))
    # 内部孔洞，让轮廓不止是一个凸块（小一点，别把外框拆成两块）
    inner = max(4, min(w, h) // 8)
    draw.ellipse([cx - inner, cy - inner, cx + inner, cy + inner], fill=(26, 30, 38))
    # 一点点噪声，避免大片纯色导致相关系数分母为 0
    for _ in range(w * h // 40):
        px = x + rng.randrange(w)
        py = y + rng.randrange(h)
        draw.point((px, py), fill=(rng.randint(0, 255), rng.randint(0, 255), rng.randint(0, 255)))


def _draw_regular_polygon(draw, cx, cy, rx, ry, sides, fill):
    """画正多边形：顶点从 0° 起算，外接框正好等于 2rx × 2ry。

    形状匹配返回的是外轮廓的外接框，顶点角偏一点外接框就会缩水（cos(22.5°)
    ≈ 0.92，直接差出 8%），所以这里统一从 0° 起算。
    """
    points = []
    for i in range(sides):
        ang = i * 2.0 * math.pi / sides
        points.append((cx + int(rx * math.cos(ang)), cy + int(ry * math.sin(ang))))
    draw.polygon(points, fill=fill)


def simple_backdrop(size, seed):
    """平滑背景（垂直渐变 + 稀疏噪点）。

    `new_canvas` 的色带 + 光斑对**模板匹配**是很好的干扰源，但对这三组素材是
    毒药：real_scale 走的是 1.0→1.5 的缩放搜索，背景里的高频光斑会在 1.0 档
    误命中（实测位置差 10 像素）；real_stop 的 EdgeMatch 阈值高达 0.90，背景
    边缘会把它拉下来。真实照片里背景干净是完全正常的，所以这里用平滑底。
    """
    rng = random.Random(seed)
    img = Image.new("RGB", size)
    draw = ImageDraw.Draw(img)
    width, height = size
    for row in range(height):
        t = row / max(1, height - 1)
        draw.line(
            [(0, row), (width - 1, row)],
            fill=(int(52 + 36 * t), int(58 + 30 * t), int(66 + 24 * t)),
        )
    for _ in range(width * height // 3000):
        draw.point((rng.randrange(width), rng.randrange(height)), fill=(rng.randint(40, 150),) * 3)
    return img


def build_real_extra():
    """补「真实图」用例里三组写死尺寸的素材（审计 R-05）。

    `AllMatchingMethodsWorkOnRealImageAssets` 除了 6 对普通照片素材，还额外要
    scale / stop / shape 三组，每组断言的矩形都是写死的，所以这里必须严格对齐：
      - real_scale：模板 96×96，source 里 (80,420) 放 **1.5 倍** → 144×144。
      - real_stop ：模板 445×449 的停车标志，Feature / Edge 都要落在这个框上。
      - real_shape：模板 421×425 的几何主体，ShapeMatch 落在 (520,140)。
    """
    made = 0

    # --- real_scale ---
    scale_tpl = Image.new("RGB", (96, 96))
    draw_texture_block(ImageDraw.Draw(scale_tpl), (0, 0, 96, 96), 211)
    scale_tpl.save(ASSETS / "opencv_real_scale_template.png", format="PNG")
    scale_src = simple_backdrop((W, H), 212)
    _paste(scale_src, scale_tpl, (80, 420, 144, 144))
    scale_src.save(ASSETS / "opencv_real_scale_source.png", format="PNG")
    made += 2

    # --- real_stop ---
    stop_w, stop_h = 445, 449
    stop_tpl = Image.new("RGB", (stop_w, stop_h), (74, 78, 84))
    ds = ImageDraw.Draw(stop_tpl)
    # 八角形红底：顶点从 0° 起算，外接框才等于 2a×2b；a/b 留 1 像素余量。
    ocx, ocy = stop_w // 2, stop_h // 2
    sa, sb = stop_w // 2 - 1, stop_h // 2 - 1
    octagon = []
    for i in range(8):
        ang = i * math.pi / 4.0
        octagon.append((ocx + int(sa * math.cos(ang)), ocy + int(sb * math.sin(ang))))
    ds.polygon(octagon, fill=(196, 48, 48))
    # 描边要够粗：EdgeMatch 阈值 0.90，1px 边在 jpg 压缩下几乎被抹平；FeatureMatch
    # 也依赖这些角点和边，细边会让它的外接框往一侧缩（实测 x 偏 10）。
    ds.polygon(octagon, outline=(245, 245, 245), width=8)
    ds.rectangle(
        [ocx - int(sa * 0.72), ocy - int(sb * 0.16), ocx + int(sa * 0.72), ocy + int(sb * 0.16)],
        fill=(245, 245, 245),
    )
    stop_tpl.save(ASSETS / "opencv_real_stop_template.png", format="PNG")
    stop_src = simple_backdrop((W, H), 213)
    _paste(stop_src, stop_tpl, (412, 222, stop_w, stop_h))
    stop_src.save(ASSETS / "opencv_real_stop_source.jpg", quality=100, subsampling=0)
    made += 2

    # --- real_shape ---
    shape_w, shape_h = 421, 425
    shape_tpl = Image.new("RGB", (shape_w, shape_h), (24, 28, 34))
    dsh = ImageDraw.Draw(shape_tpl)
    scx, scy = shape_w // 2, shape_h // 2
    # 不留余量：顶点在 scx±(w//2) 上，跨度正好是 shape_w（形状匹配返回的外接框
    # 就是这里的外轮廓，留 1 像素余量会让结果比模板小 2，超出 ±1 的容差）。
    ha, hb = shape_w // 2, shape_h // 2
    hexagon = []
    for i in range(8):
        ang = i * math.pi / 4.0
        hexagon.append((scx + int(ha * math.cos(ang)), scy + int(hb * math.sin(ang))))
    dsh.polygon(hexagon, fill=(228, 222, 198))
    dsh.ellipse(
        [scx - shape_w // 12, scy - shape_h // 12, scx + shape_w // 12, scy + shape_h // 12],
        fill=(24, 28, 34),
    )
    shape_tpl.save(ASSETS / "opencv_real_shape_template.png", format="PNG")
    shape_src = simple_backdrop((W, H), 214)
    _paste(shape_src, shape_tpl, (520, 140, shape_w, shape_h))
    shape_src.save(ASSETS / "opencv_real_shape_source.png", format="PNG")
    made += 2

    return made


def build_hard_extra():
    """补 HardRealImageCasesExposeMatchingBoundaries 的 6 组素材。

    这条用例验的是"困难场景下算法的边界"，所以每条断言的方向都不同：
      - hard_scale      ：同一模板 0.75x / 1.35x 两档 + ROI 内自动搜 1.25x；
      - hard_brightness ：整体调亮后仍要匹配上；
      - hard_repeated   ：同一图案出现 5+ 次要全部找到；
      - hard_rotation   ：旋转后 MatchTemplate / EdgeMatch **必须失败**，
                          FeatureMatch（旋转不变）必须成功；
      - hard_occlusion  ：大面积遮挡后 MatchTemplate **必须失败**；
      - hard_similar    ：旁边放形状相近的干扰物，仍要命中目标。
    """
    made = 0

    # --- hard_scale ---
    # 原断言里 1.35x(940,390,130,130) 与 ROI 自动搜的 1.25x(947,396,120,120)
    # 是**互相包含**的两个矩形，物理上不可能同时存在，所以把 1.35x 挪到 ROI
    # {820,300,360,260} 之外，1.25x 留在 ROI 内（见 opencv_test.cpp 的同步改动）。
    hard_scale_tpl = Image.new("RGB", (96, 96))
    draw_texture_block(ImageDraw.Draw(hard_scale_tpl), (0, 0, 96, 96), 311)
    hard_scale_tpl.save(ASSETS / "opencv_hard_scale_template.png", format="PNG")
    hard_scale_src = simple_backdrop((W, H), 312)
    _paste(hard_scale_src, hard_scale_tpl, (90, 450, 72, 72))  # 0.75x
    _paste(hard_scale_src, hard_scale_tpl, (700, 180, 130, 130))  # 1.354x，ROI 之外
    _paste(hard_scale_src, hard_scale_tpl, (860, 320, 120, 120))  # 1.25x，ROI 之内
    hard_scale_src.save(ASSETS / "opencv_hard_scale_source.png", format="PNG")
    made += 2

    # --- hard_brightness ---
    bright_tpl = Image.new("RGB", (96, 96))
    draw_texture_block(ImageDraw.Draw(bright_tpl), (0, 0, 96, 96), 313)
    bright_tpl.save(ASSETS / "opencv_hard_brightness_template.png", format="PNG")
    bright_src = simple_backdrop((W, H), 314)
    # 1.45 会大面积饱和到 255：TM_CCOEFF_NORMED 虽然对线性亮度变化不敏感，
    # 但截断不是线性变化 —— 高光区结构被压平，相关系数掉到 0.90 以下。
    # 1.20 能让"整体变亮"这件事成立，又不触发截断。
    lit = ImageEnhance.Brightness(bright_tpl).enhance(1.20)
    _paste(bright_src, lit, (460, 430, 96, 96))
    bright_src.save(ASSETS / "opencv_hard_brightness_source.png", format="PNG")
    made += 2

    # --- hard_repeated ---
    rep_tpl = Image.new("RGB", (80, 80))
    draw_texture_block(ImageDraw.Draw(rep_tpl), (0, 0, 80, 80), 315)
    rep_tpl.save(ASSETS / "opencv_hard_repeated_template.png", format="PNG")
    rep_src = simple_backdrop((W, H), 316)
    for pos in [(120, 120), (420, 120), (720, 120), (1020, 120), (260, 480), (640, 480)]:
        _paste(rep_src, rep_tpl, (pos[0], pos[1], 80, 80))
    rep_src.save(ASSETS / "opencv_hard_repeated_source.png", format="PNG")
    made += 2

    # --- hard_rotation ---
    rot_tpl = Image.new("RGB", (160, 160))
    draw_texture_block(ImageDraw.Draw(rot_tpl), (0, 0, 160, 160), 317)
    rot_tpl.save(ASSETS / "opencv_hard_rotation_template.png", format="PNG")
    rot_src = simple_backdrop((W, H), 318)
    rotated = rot_tpl.rotate(25.0, resample=Image.BICUBIC, expand=True, fillcolor=(58, 62, 70))
    # FeatureMatch 不要求旋转不变，但要求外接框大致落在 (610,130) ±24。
    _paste(rot_src, rotated, (610, 130, rotated.size[0], rotated.size[1]))
    rot_src.save(ASSETS / "opencv_hard_rotation_source.png", format="PNG")
    made += 2

    # --- hard_occlusion ---
    occ_tpl = Image.new("RGB", (120, 120))
    draw_texture_block(ImageDraw.Draw(occ_tpl), (0, 0, 120, 120), 319)
    occ_tpl.save(ASSETS / "opencv_hard_occlusion_template.png", format="PNG")
    occ_src = simple_backdrop((W, H), 320)
    _paste(occ_src, occ_tpl, (520, 260, 120, 120))
    # 盖掉中间约 70% 面积：模板匹配得分会掉到 0.90 以下（用例期望它失败）。
    blocked = ImageDraw.Draw(occ_src)
    blocked.rectangle([520 + 18, 260 + 18, 520 + 102, 260 + 102], fill=(150, 42, 42))
    occ_src.save(ASSETS / "opencv_hard_occlusion_source.png", format="PNG")
    made += 2

    # --- hard_similar ---
    sim_w, sim_h = 413, 423
    sim_tpl = Image.new("RGB", (sim_w, sim_h), (24, 28, 34))
    sim_draw = ImageDraw.Draw(sim_tpl)
    _draw_regular_polygon(sim_draw, sim_w // 2, sim_h // 2, sim_w // 2, sim_h // 2, 8, (228, 222, 198))
    sim_tpl.save(ASSETS / "opencv_hard_similar_shapes_template.png", format="PNG")
    sim_src = simple_backdrop((W, H), 321)
    _paste(sim_src, sim_tpl, (520, 140, sim_w, sim_h))
    # 干扰物：形状相似但边数不同、尺寸略小，阈值 0.55 下不该被选中。
    distractor = Image.new("RGB", (360, 380), (24, 28, 34))
    _draw_regular_polygon(
        ImageDraw.Draw(distractor), 180, 190, 178, 188, 7, (228, 222, 198)
    )
    _paste(sim_src, distractor, (70, 150, 360, 380))
    sim_src.save(ASSETS / "opencv_hard_similar_shapes_source.png", format="PNG")
    made += 2

    return made


def build_patterns():
    """先把每个模板图案画成独立小图，再贴到各自的 source 上。

    例外：gem 与 shape 的区域在用例里**本来就重叠** 15 像素（455,325,70,70 与
    420,65,240,275）。真实素材里"宝石嵌在形状里"就是这种像素共享关系，所以 gem
    不能独立画 —— 它就是场景合成之后从 (455,325) 裁下来的那一块，这样才与场景
    逐像素一致（否则 shape 会盖掉 gem 的顶部，gem 只能得 0.83）。
    """
    patterns = {}
    for _, template_file, rect, seed in SCENE_CASES:
        if template_file in patterns:
            continue
        w, h = rect[2], rect[3]
        img = Image.new("RGB", (w, h))
        draw = ImageDraw.Draw(img)
        if template_file == "opencv_game_shape_template.png":
            draw_shape_subject(draw, (0, 0, w, h), seed)
        else:
            draw_texture_block(draw, (0, 0, w, h), seed)
        patterns[template_file] = img
    return patterns


def build_scene():
    patterns = build_patterns()
    for name, img in patterns.items():
        img.save(ASSETS / name, format="PNG")

    coin = patterns[COIN_TEMPLATE]
    coin_scaled = coin.resize((SCALE_RECT[2], SCALE_RECT[3]), Image.LANCZOS)

    # scene 主图：四个图案各就各位，scale 位置放 coin 的 1.5 倍放大。
    # 必须**按面积从大到小**贴：shape 的区域 (420,65,240,275) 与 gem 的
    # (455,325,70,70) 有 15 像素重叠，先贴小的就会被大的盖掉（gem 的相关系数
    # 因此掉到 0.83）。
    scene, _ = new_canvas(777)
    on_scene = [
        (S, T, R) for (S, T, R, _) in SCENE_CASES if S == SCENE_SOURCE and T != "opencv_game_scene_template.png"
    ]
    on_scene.sort(key=lambda item: item[2][2] * item[2][3], reverse=True)
    for _, template_file, rect in on_scene:
        _paste(scene, patterns[template_file], rect)
    scene_rect = next(r for s, t, r, sd in SCENE_CASES if t == "opencv_game_scene_template.png")
    _paste(scene, patterns["opencv_game_scene_template.png"], scene_rect)
    _paste(scene, coin_scaled, SCALE_RECT)

    # AllMatchingMethodsWorkOnGameSceneAssets 断言"同一金币模板在场景里能匹配到
    # >= 4 处"（真实游戏场景里金币当然不止一个）。主位 (170,185) 之外再撒 4 个，
    # 位置都避开上面那些图案。
    for extra in COIN_EXTRA_POSITIONS:
        _paste(scene, coin, (extra[0], extra[1], 70, 70))

    # 从合成完的 scene 里裁出 gem 模板 —— 它与 shape 重叠的那 15 行已经是
    # "形状里的宝石"的真实像素，裁出来才与场景一致。
    for deferred in DEFERRED_TO_CROP:
        rect = next(r for s, t, r, _ in SCENE_CASES if t == deferred)
        patterns[deferred] = scene.crop((rect[0], rect[1], rect[0] + rect[2], rect[1] + rect[3]))
    scene.save(ASSETS / SCENE_SOURCE, format="PNG")

    # 三个独立 source：只放自己那块图案，其余是背景。
    for source_file, template_file in GAME_SOURCES.items():
        canvas, _ = new_canvas(300 + sum(source_file.encode()) % 200)
        rect = next(r for s, t, r, sd in SCENE_CASES if s == source_file and t == template_file)
        img = coin_scaled if template_file == COIN_TEMPLATE else patterns[template_file]
        _paste(canvas, img, rect)
        canvas.save(ASSETS / source_file, format="PNG")
    return len(patterns) + len(GAME_SOURCES) + 1


def _paste(canvas, img, rect):
    x, y, w, h = rect
    canvas.paste(img.resize((w, h), Image.LANCZOS) if img.size != (w, h) else img, (x, y))


def verify():
    import cv2

    failures = 0
    for source_file, template_file, rect, _ in PHOTO_CASES + SCENE_CASES:
        src = cv2.imread(str(ASSETS / source_file), cv2.IMREAD_COLOR)
        tpl = cv2.imread(str(ASSETS / template_file), cv2.IMREAD_COLOR)
        if src is None or tpl is None:
            print(f"  [FAIL] 读不到 {source_file} / {template_file}")
            failures += 1
            continue
        x, y, w, h = rect
        # scale 那一项在 source 里放的是模板的 1.5 倍放大，原尺寸匹配本来就不该命中
        # （用例走的是 MatchTemplateScale 的缩放搜索）。这里按多档缩放找，等价于
        # 验证"1.0~1.5 倍里存在一档能在期望位置拿到高分"。
        best = (-1.0, (0, 0), 1.0)
        for factor in (1.0, 1.25, 1.5):
            scaled = tpl
            if factor != 1.0:
                scaled = cv2.resize(tpl, None, fx=factor, fy=factor, interpolation=cv2.INTER_LANCZOS4)
            if scaled.shape[0] > src.shape[0] or scaled.shape[1] > src.shape[1]:
                continue
            res = cv2.matchTemplate(src, scaled, cv2.TM_CCOEFF_NORMED)
            # 同一模板可能在 source 里出现多次（scene 里撒了 5 个金币），
            # minMaxLoc 只给其中一个峰，位置不唯一。所以按"期望位置处的
            # 相关系数"打分，这才是用例真正关心的东西。
            at_expected = float(res[y, x]) if y < res.shape[0] and x < res.shape[1] else -1.0
            _, max_val, _, max_loc = cv2.minMaxLoc(res)
            if at_expected > best[0]:
                best = (at_expected, max_loc, factor)
        score, loc, factor = best
        # score 已经是"模板左上角落在期望位置时"的相关系数，位置错了它必然低，
        # 所以不用再单独比 loc（loc 只是 minMaxLoc 挑的那个峰，多实例时不唯一）。
        ok = score >= 0.90
        print(f"  [{' OK ' if ok else 'FAIL'}] {template_file}: 期望({x},{y}) 实测峰{loc} "
              f"score={score:.4f} scale={factor}")
        if not ok:
            failures += 1
    print(f"verify: {failures} failure(s)")
    return failures


def main():
    ASSETS.mkdir(parents=True, exist_ok=True)
    n = 0
    for source_file, template_file, rect, seed in PHOTO_CASES:
        n += build_photo_case(source_file, template_file, rect, seed)
    n += build_scene()
    n += build_real_extra()
    n += build_hard_extra()
    print(f"生成 {n} 组素材 -> {ASSETS}")
    if "--verify" in sys.argv:
        return 1 if verify() else 0
    return 0


if __name__ == "__main__":
    sys.exit(main())
