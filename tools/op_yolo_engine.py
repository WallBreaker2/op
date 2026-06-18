#!/usr/bin/env python3
"""Minimal YOLO HTTP engine for OP.

Protocol:
  POST /api/v1/detect
  {"image":"base64 raw pixels","width":800,"height":600,"bpp":4,"conf":0.25,"iou":0.45}

Response:
  {"code":0,"results":[{"class_id":0,"label":"...","bbox":[x1,y1,x2,y2],"confidence":0.95}]}
"""

from __future__ import annotations

import argparse
import base64
import json
import sys
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from typing import Any

try:
    from PIL import Image
except ImportError as exc:  # pragma: no cover - startup guard
    raise SystemExit("Missing dependency: pip install pillow") from exc

try:
    from ultralytics import YOLO
except ImportError as exc:  # pragma: no cover - startup guard
    raise SystemExit("Missing dependency: pip install ultralytics") from exc


class YoloEngine:
    def __init__(self, model_path: str) -> None:
        self.model = YOLO(model_path)
        self.names = getattr(self.model, "names", {}) or {}

    def detect(self, image: Image.Image, conf: float, iou: float) -> list[dict[str, Any]]:
        results = self.model.predict(image, conf=conf, iou=iou, verbose=False)
        if not results:
            return []

        detections: list[dict[str, Any]] = []
        boxes = getattr(results[0], "boxes", None)
        if boxes is None:
            return detections

        for box in boxes:
            xyxy = box.xyxy[0].tolist()
            class_id = int(box.cls[0].item()) if box.cls is not None else -1
            confidence = float(box.conf[0].item()) if box.conf is not None else 0.0
            label = str(self.names.get(class_id, class_id))
            detections.append(
                {
                    "class_id": class_id,
                    "label": label,
                    "bbox": [int(round(v)) for v in xyxy],
                    "confidence": confidence,
                }
            )
        return detections


def decode_raw_image(payload: dict[str, Any]) -> Image.Image:
    width = int(payload["width"])
    height = int(payload["height"])
    bpp = int(payload.get("bpp", 4))
    raw = base64.b64decode(payload["image"], validate=True)
    expected = width * height * bpp
    if width <= 0 or height <= 0 or bpp not in (1, 3, 4):
        raise ValueError("invalid width/height/bpp")
    if len(raw) != expected:
        raise ValueError(f"invalid image byte length: got {len(raw)}, expected {expected}")

    if bpp == 4:
        # OP Image stores 4-byte pixels in BGRA order on Windows.
        return Image.frombytes("RGBA", (width, height), raw, "raw", "BGRA").convert("RGB")
    if bpp == 3:
        return Image.frombytes("RGB", (width, height), raw, "raw", "BGR")
    return Image.frombytes("L", (width, height), raw).convert("RGB")


def make_handler(engine: YoloEngine, max_body_bytes: int):
    class Handler(BaseHTTPRequestHandler):
        server_version = "op-yolo-engine/0.1"

        def do_GET(self) -> None:  # noqa: N802
            if self.path in ("/", "/health"):
                self._send_json(200, {"code": 0, "status": "ok"})
                return
            self._send_json(404, {"code": 404, "message": "not found"})

        def do_POST(self) -> None:  # noqa: N802
            if self.path.rstrip("/") != "/api/v1/detect":
                self._send_json(404, {"code": 404, "message": "not found"})
                return

            try:
                content_length = int(self.headers.get("Content-Length", "0"))
                if content_length <= 0 or content_length > max_body_bytes:
                    raise ValueError("invalid Content-Length")
                payload = json.loads(self.rfile.read(content_length).decode("utf-8"))
                image = decode_raw_image(payload)
                conf = float(payload.get("conf", 0.25))
                iou = float(payload.get("iou", 0.45))
                detections = engine.detect(image, conf=conf, iou=iou)
                self._send_json(200, {"code": 0, "results": detections})
            except Exception as exc:  # keep batch callers alive with structured errors
                self._send_json(400, {"code": -1, "message": str(exc), "results": []})

        def log_message(self, fmt: str, *args: Any) -> None:
            print(f"{self.address_string()} - {fmt % args}", file=sys.stderr)

        def _send_json(self, status: int, body: dict[str, Any]) -> None:
            data = json.dumps(body, ensure_ascii=False, separators=(",", ":")).encode("utf-8")
            self.send_response(status)
            self.send_header("Content-Type", "application/json; charset=utf-8")
            self.send_header("Content-Length", str(len(data)))
            self.end_headers()
            self.wfile.write(data)

    return Handler


def main() -> int:
    parser = argparse.ArgumentParser(description="Minimal OP YOLO HTTP detection engine")
    parser.add_argument("--model", required=True, help="YOLO model path, e.g. models/yolo/best.pt or best.onnx")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8090)
    parser.add_argument("--max-body-mb", type=int, default=96)
    args = parser.parse_args()

    engine = YoloEngine(args.model)
    handler = make_handler(engine, max_body_bytes=args.max_body_mb * 1024 * 1024)
    server = ThreadingHTTPServer((args.host, args.port), handler)
    print(f"op_yolo_engine listening on http://{args.host}:{args.port}/api/v1/detect", file=sys.stderr)
    print(f"model={args.model}", file=sys.stderr)
    server.serve_forever()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
