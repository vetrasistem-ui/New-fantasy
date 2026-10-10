"""Attribute widths mirror Shared/Formats/Legacy/OtbmStreamReader.cpp."""
import struct


class Cursor:
    def __init__(self, data):
        self.data, self.pos = data, 0

    def number(self, fmt):
        size = struct.calcsize('<' + fmt)
        if self.pos + size > len(self.data):
            raise ValueError(f"Truncated OTBM property at {self.pos}")
        value = struct.unpack_from('<' + fmt, self.data, self.pos)[0]
        self.pos += size
        return value

    def string(self):
        size = self.number('H')
        if self.pos + size > len(self.data):
            raise ValueError("Truncated OTBM string")
        raw = self.data[self.pos:self.pos + size]
        self.pos += size
        # Latin-1 is reversible; no source bytes are silently replaced.
        return raw.decode('latin-1')

    def position(self):
        return [self.number('H'), self.number('H'), self.number('B')]

    def finish(self):
        if self.pos != len(self.data):
            raise ValueError(f"Unexpected trailing properties at {self.pos}")


ITEM_ATTRIBUTES = {
    4: ('actionId', 'H'), 5: ('uniqueId', 'H'), 6: ('text', 'string'),
    7: ('description', 'string'), 8: ('teleportDestination', 'position'),
    10: ('depotId', 'H'), 12: ('runeCharges', 'B'), 14: ('houseDoorId', 'B'),
    15: ('count', 'B'), 16: ('duration', 'i'), 17: ('decayingState', 'B'),
    18: ('writtenDate', 'I'), 19: ('writtenBy', 'string'),
    20: ('sleeperGuid', 'I'), 21: ('sleepStart', 'I'), 22: ('charges', 'H'),
}


def item_attributes(cursor):
    values = []
    while cursor.pos < len(cursor.data):
        start = cursor.pos
        code = cursor.number('B')
        spec = ITEM_ATTRIBUTES.get(code)
        if not spec:
            # Width unknown: retain the whole remaining payload, do not guess.
            return values, (code, cursor.data[start:])
        name, fmt = spec
        value = (cursor.string() if fmt == 'string' else
                 cursor.position() if fmt == 'position' else cursor.number(fmt))
        values.append((name, value))
    return values, None
