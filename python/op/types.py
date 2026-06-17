from enum import IntEnum

__all__ = [
    "IntType",
    "StringType",
]


class IntType(IntEnum):
    I32 = 0
    I16 = 1
    I8 = 2
    I64 = 3
    U32 = 4
    U16 = 5
    U8 = 6


class StringType(IntEnum):
    ANSI = 0
    GBK = 0
    UTF16 = 1
    UTF8 = 2
