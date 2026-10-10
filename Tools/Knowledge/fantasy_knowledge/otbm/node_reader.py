"""Bounded-buffer physical node reader; never materializes a map tree."""
from dataclasses import dataclass

START, END, ESCAPE = 254, 255, 253


@dataclass(frozen=True)
class Node:
    kind: int
    properties: bytes
    offset: int
    depth: int


class NodeReader:
    def __init__(self, stream, chunk_size=1024 * 1024):
        self.stream = stream
        self.chunk_size = chunk_size
        self.buffer = b""
        self.index = 0
        self.offset = 0

    def byte(self):
        if self.index == len(self.buffer):
            self.buffer = self.stream.read(self.chunk_size)
            self.index = 0
            if not self.buffer:
                raise ValueError(f"Truncated OTBM at byte {self.offset}")
        value = self.buffer[self.index]
        self.index += 1
        self.offset += 1
        return value

    def events(self):
        identifier = bytes(self.byte() for _ in range(4))
        if identifier not in (b"\0\0\0\0", b"OTBM"):
            raise ValueError("Unsupported OTBM identifier")
        marker = self.byte()
        stack = []
        while True:
            if marker == START:
                offset = self.offset - 1
                kind = self.byte()
                properties = bytearray()
                while True:
                    marker = self.byte()
                    if marker in (START, END):
                        break
                    properties.append(self.byte() if marker == ESCAPE else marker)
                    if len(properties) > 16 * 1024 * 1024:
                        raise ValueError(f"Oversized node properties at {offset}")
                if len(stack) >= 128:
                    raise ValueError("OTBM nesting exceeds 128")
                node = Node(kind, bytes(properties), offset, len(stack))
                stack.append(node)
                yield "start", node
            elif marker == END:
                if not stack:
                    raise ValueError("Unbalanced OTBM terminator")
                yield "end", stack.pop()
                if not stack:
                    if self.index < len(self.buffer) or self.stream.read(1):
                        raise ValueError("Trailing bytes after OTBM root")
                    return
                marker = self.byte()
            else:
                raise ValueError(f"Unexpected node marker {marker} at {self.offset - 1}")
