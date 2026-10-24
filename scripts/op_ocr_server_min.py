#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""最小可用的 OCR HTTP 服务（参考实现，供 OP 的 HTTP OCR 客户端联调/跑测试）。

协议（与 libop/ocr/OcrService.cpp 对齐）：
    GET  /health          -> {"status":"ok","backend":"<name>"}
    POST /api/v1/ocr      -> 请求 {"image":"<base64>","width":W,"height":H,"bpp":4}
                             响应 {"code":0,"results":[{"text":"..","bbox":[l,t,r,b],"confidence":0.98}]}

后端：
    tesseract  需要本机装 Tesseract 可执行文件 + pip install pytesseract pillow
    echo       不需要任何三方依赖，固定返回一段文本，**仅用于验证协议/联调**，不做真实识别

用法：
    python scripts/op_ocr_server_min.py --backend echo --port 8082
    python scripts/op_ocr_server_min.py --backend tesseract --lang eng --port 8082
    python scripts/op_ocr_server_min.py --selftest          # 不需要起服务，验证自身逻辑
"""

import argparse
import base64
import json
import sys
import threading
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

API_PATH = "/api/v1/ocr"
HEALTH_PATH = "/health"


def make_echo_backend():
    def recognize(_image, _width, _height):
        # 固定返回，只为让客户端走到"解析结果"这条分支。
        return [{"text": "OP", "bbox": [0, 0, 10, 10], "confidence": 0.99}]

    return recognize


def make_tesseract_backend(lang):
    try:
        import pytesseract  # noqa: F401
        from PIL import Image
    except Exception as exc:  # 缺依赖时明确报错，不要静默
        raise SystemExit(
            "tesseract 后端需要: pip install pytesseract pillow，并确保 tesseract.exe 在 PATH 中\n原始错误: %s" % exc
        )

    def recognize(image_bytes, width, height):
        from PIL import Image
        import io
        import pytesseract

        image = Image.frombytes("RGBA", (width, height), image_bytes).convert("RGB")
        data = pytesseract.image_to_data(image, lang=lang, output_type=pytesseract.Output.DICT)
        results = []
        for i, text in enumerate(data.get("text", [])):
            if not text or not text.strip():
                continue
            x, y = data["left"][i], data["top"][i]
            w, h = data["width"][i], data["height"][i]
            try:
                conf = float(data["conf"][i]) / 100.0
            except (TypeError, ValueError):
                conf = 0.0
            results.append({"text": text.strip(), "bbox": [x, y, x + w, y + h], "confidence": conf})
        return results

    return recognize


class OcrHandler(BaseHTTPRequestHandler):
    backend_name = "echo"
    recognize = staticmethod(lambda *_: [])

    def log_message(self, fmt, *args):  # 安静一点，避免刷屏
        pass

    def _send(self, code, payload):
        body = json.dumps(payload).encode("utf-8")
        self.send_response(code)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self):
        if self.path.startswith(HEALTH_PATH):
            self._send(200, {"status": "ok", "backend": self.backend_name})
        else:
            self._send(404, {"code": -1, "error": "not found"})

    def do_POST(self):
        if not self.path.startswith(API_PATH):
            self._send(404, {"code": -1, "error": "not found"})
            return
        try:
            length = int(self.headers.get("Content-Length", "0"))
            request = json.loads(self.rfile.read(length).decode("utf-8"))
            width = int(request["width"])
            height = int(request["height"])
            bpp = int(request.get("bpp", 4))
            raw = base64.b64decode(request["image"])
            expected = width * height * bpp
            if len(raw) < expected:
                self._send(200, {"code": -2, "error": "image bytes shorter than width*height*bpp"})
                return
            results = self.recognize(raw[:expected], width, height)
            self._send(200, {"code": 0, "results": results})
        except Exception as exc:  # 任何异常都回 200 + code!=0，客户端据此判失败
            self._send(200, {"code": -1, "error": str(exc)})


def selftest(backend_name, lang):
    """不联网、不起服务，直接验证后端与 JSON 形状。"""
    if backend_name == "tesseract":
        recognize = make_tesseract_backend(lang)
    else:
        recognize = make_echo_backend()
    results = recognize(b"\xff\xff\xff\xff" * (8 * 8), 8, 8)
    payload = {"code": 0, "results": results}
    text = json.dumps(payload)
    assert '"code": 0' in text, text
    assert "results" in text, text
    print("selftest ok: backend=%s results=%d" % (backend_name, len(results)))
    return 0


def main(argv=None):
    parser = argparse.ArgumentParser()
    parser.add_argument("--backend", default="echo", choices=["echo", "tesseract"])
    parser.add_argument("--lang", default="eng")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8082)
    parser.add_argument("--selftest", action="store_true")
    args = parser.parse_args(argv)

    if args.selftest:
        return selftest(args.backend, args.lang)

    recognize = make_tesseract_backend(args.lang) if args.backend == "tesseract" else make_echo_backend()
    OcrHandler.backend_name = args.backend
    OcrHandler.recognize = staticmethod(recognize)

    server = ThreadingHTTPServer((args.host, args.port), OcrHandler)
    print("op ocr server listening on http://%s:%d (backend=%s)" % (args.host, args.port, args.backend))
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    try:
        while True:
            thread.join(1)
    except KeyboardInterrupt:
        server.shutdown()
    return 0


if __name__ == "__main__":
    sys.exit(main())
