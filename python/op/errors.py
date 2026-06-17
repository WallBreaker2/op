class OpError(RuntimeError):
    """Base class for Python wrapper errors."""


class DllLoadError(OpError):
    """Raised when the native C API DLL cannot be loaded."""


class ClosedHandleError(OpError):
    """Raised when an Op instance is used after close()."""


class OpCallError(OpError):
    """Raised when a native call reports failure."""

    def __init__(self, name: str):
        super().__init__(f"{name} failed")
        self.name = name
