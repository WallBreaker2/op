# YOLO HTTP 检测

OP 的 YOLO 集成采用与通用 OCR 相同的外部 HTTP 服务模式：`op_x64.dll` / `op_x86.dll` 负责截图、读图和 HTTP 调用，YOLO11/YOLOv11 的模型加载与推理由独立服务完成。

## 接口

```text
SetYoloEngine(path_of_engine, dll_name, argv) -> 1/0
YoloDetect(x1, y1, x2, y2, conf, iou) -> ret + JSON
YoloDetectFromFile(file_name, conf, iou) -> ret + JSON
```

- `SetYoloEngine` 支持完整 URL、基础 URL 或别名：`yolo`、`yolo11`、`yolov11`、`yolo_http`、`yolo_server`。
- 默认地址：`http://127.0.0.1:8090/api/v1/detect`。
- 基础 URL 会自动补全 `/api/v1/detect`，例如 `http://127.0.0.1:8090`。
- `argv` 支持：`--url=http://...`、`--timeout=5000`。
- 环境变量：`OP_YOLO_URL`、`OP_YOLO_BACKEND`、`OP_YOLO_TIMEOUT_MS`。

## 最小服务样例

仓库提供了一个最小 Python HTTP 服务：

```powershell
pip install ultralytics pillow
python tools/op_yolo_engine.py --model models/yolo/best.pt --host 127.0.0.1 --port 8090
```

健康检查：

```powershell
curl http://127.0.0.1:8090/health
```

如果使用 ONNX 模型，可把 `--model` 改成对应 `.onnx` 路径；实际可用后端取决于本机 Ultralytics/运行时环境。

## HTTP 请求

OP 发送原始像素的 base64，不在 DLL 内做 YOLO 推理：

```json
{
  "image": "base64原始像素",
  "width": 800,
  "height": 600,
  "bpp": 4,
  "conf": 0.25,
  "iou": 0.45
}
```

## HTTP 返回

服务应返回：

```json
{
  "code": 0,
  "results": [
    {
      "class_id": 0,
      "label": "button",
      "bbox": [10, 20, 100, 120],
      "confidence": 0.95
    }
  ]
}
```

兼容字段：

- 类名字段：`label`、`class_name`、`name`
- 类 ID 字段：`class_id`、`cls`
- 置信度字段：`confidence`、`conf`

## OP 返回

`YoloDetect` 返回的坐标会转换为屏幕/绑定窗口区域坐标；`YoloDetectFromFile` 返回文件内坐标。

返回 JSON 形态：

```json
{"code":0,"results":[{"class_id":0,"label":"button","bbox":[10,20,100,120],"confidence":0.950000}]}
```

`ret` 为检测数量；请求失败、文件不存在或未绑定截图时 `ret=0` 且 JSON 为空。

## Python COM 示例

```python
from win32com.client import Dispatch

op = Dispatch("op.opsoft")
op.SetYoloEngine("http://127.0.0.1:8090", "", "--timeout=5000")
ret, json_text = op.YoloDetectFromFile(r"E:\\test.bmp", 0.25, 0.45)
print(ret, json_text)
```

> 具体 COM 语言绑定对 `[out] BSTR* retjson` 和 `[retval] LONG* ret` 的返回形式可能不同，以宿主语言实际封装为准。
