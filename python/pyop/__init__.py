"""OP Windows automation plugin — Python bindings."""

import os

_PKG_DIR = os.path.dirname(os.path.abspath(__file__))

if hasattr(os, "add_dll_directory"):
    os.add_dll_directory(_PKG_DIR)

from ._binding import Op

__all__ = ["Op"]
