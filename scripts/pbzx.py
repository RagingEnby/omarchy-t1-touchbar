#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later
"""Extract a PBZX Payload stream to stdout without a libxar dependency."""

from __future__ import annotations

import lzma
import struct
import sys
from pathlib import Path

MORE_CHUNKS = 1 << 24
PLAIN_CHUNK_SIZE = 0x1000000
XZ_MAGIC = b"\xfd7zXZ\x00"


def read_exact(stream, size: int) -> bytes:
    data = stream.read(size)
    if len(data) != size:
        raise ValueError(f"truncated PBZX stream: wanted {size} bytes")
    return data


def read_u64(stream) -> int:
    return struct.unpack(">Q", read_exact(stream, 8))[0]


def extract(path: Path) -> None:
    with path.open("rb") as stream:
        if read_exact(stream, 4) != b"pbzx":
            raise ValueError("input is not a PBZX stream")

        flags = read_u64(stream)
        output = sys.stdout.buffer
        chunks = 0

        while flags & MORE_CHUNKS:
            flags = read_u64(stream)
            length = read_u64(stream)
            payload = read_exact(stream, length)
            chunks += 1

            if length == PLAIN_CHUNK_SIZE:
                output.write(payload)
            else:
                if not payload.startswith(XZ_MAGIC):
                    raise ValueError(f"chunk {chunks} has no XZ header")
                try:
                    output.write(lzma.decompress(payload, format=lzma.FORMAT_XZ))
                except lzma.LZMAError as error:
                    raise ValueError(f"chunk {chunks} XZ decode failed: {error}") from error

        output.flush()
        if chunks == 0:
            raise ValueError("PBZX stream contained no chunks")


def main() -> int:
    if len(sys.argv) != 2:
        print(f"usage: {Path(sys.argv[0]).name} Payload", file=sys.stderr)
        return 2
    try:
        extract(Path(sys.argv[1]))
    except (OSError, ValueError) as error:
        print(f"pbzx: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
