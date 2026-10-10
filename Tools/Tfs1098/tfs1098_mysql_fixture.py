#!/usr/bin/env python3
"""Small MySQL-protocol fixture for TFS 1.4.2 homologation.

This exists only to make cross-platform runtime/client smoke tests deterministic.
It is NOT a database implementation and must never be used for persistence or
production. Real persistence is homologated separately against MariaDB 10.11.
"""

from __future__ import annotations

import argparse
from pathlib import Path
import re
import socket
import struct
import threading

PASSWORD_SHA1 = "ec1e111db30c9cca1cca2958af3711a899cee873"  # fantasy


def p3(value: int) -> bytes:
    return bytes((value & 255, (value >> 8) & 255, (value >> 16) & 255))


def packet(payload: bytes, sequence: int) -> bytes:
    return p3(len(payload)) + bytes([sequence & 255]) + payload


def lenenc_int(value: int) -> bytes:
    if value < 0xFB:
        return bytes([value])
    if value <= 0xFFFF:
        return b"\xFC" + struct.pack("<H", value)
    if value <= 0xFFFFFF:
        return b"\xFD" + bytes((value & 255, (value >> 8) & 255, (value >> 16) & 255))
    return b"\xFE" + struct.pack("<Q", value)


def lenenc_string(value: object | None) -> bytes:
    if value is None:
        return b"\xFB"
    if isinstance(value, bytes):
        data = value
    else:
        data = str(value).encode("utf-8")
    return lenenc_int(len(data)) + data


def column_definition(name: str, type_code: int = 0xFD) -> bytes:
    parts = [b"def", b"", b"", b"", name.encode("utf-8"), b""]
    body = b"".join(lenenc_string(part) for part in parts)
    body += b"\x0C" + struct.pack("<H", 33) + struct.pack("<I", 1024)
    body += bytes([type_code]) + struct.pack("<H", 0) + b"\x00\x00\x00"
    return body


def eof_packet() -> bytes:
    return b"\xFE\x00\x00\x02\x00"


def ok_packet() -> bytes:
    return b"\x00\x00\x00\x02\x00\x00\x00"


def send_resultset(conn: socket.socket, columns: list[object], rows: list[tuple[object, ...]]) -> None:
    sequence = 1
    conn.sendall(packet(lenenc_int(len(columns)), sequence))
    sequence += 1
    for column in columns:
        if isinstance(column, tuple):
            name, type_code = column
        else:
            name, type_code = column, 0xFD
        conn.sendall(packet(column_definition(str(name), int(type_code)), sequence))
        sequence += 1
    conn.sendall(packet(eof_packet(), sequence))
    sequence += 1
    for row in rows:
        conn.sendall(packet(b"".join(lenenc_string(value) for value in row), sequence))
        sequence += 1
    conn.sendall(packet(eof_packet(), sequence))


def guessed_columns(query: str) -> list[tuple[str, int]]:
    match = re.match(r"select (.+?) from ", query, re.IGNORECASE)
    if not match:
        return [("dummy", 0xFD)]
    columns: list[tuple[str, int]] = []
    for part in match.group(1).split(","):
        name = part.strip()
        alias = re.search(r"\bas\s+`?([\w]+)`?$", name, re.IGNORECASE)
        if alias:
            name = alias.group(1)
        else:
            name = name.split(".")[-1].strip("` ")
        columns.append((name or "col", 0xFD))
    return columns


def query_response(query: str) -> tuple[list[object], list[tuple[object, ...]]] | None:
    normalized = " ".join(query.strip().split())
    lower = normalized.lower()

    if "max_allowed_packet" in lower:
        return [("Variable_name", 0xFD), ("Value", 0xFD)], [("max_allowed_packet", "67108864")]
    if "from `server_config`" in lower and "db_version" in lower:
        return [("value", 0x03)], [("30",)]
    if lower.startswith("select") and "information_schema" in lower:
        return [("TABLE_NAME", 0xFD)], [("accounts",), ("players",), ("server_config",)]

    if "from `accounts` where `name`" in lower and "secret" in lower:
        return [
            ("id", 0x03), ("name", 0xFD), ("password", 0xFD),
            ("secret", 0xFD), ("type", 0x03), ("premium_ends_at", 0x08),
        ], [("1", "fantasy", PASSWORD_SHA1, "", "1", "0")]
    if "select `name` from `players` where `account_id`" in lower:
        return [("name", 0xFD)], [("Fantasy Test",)]
    if "select `id`, `password`, `secret` from `accounts`" in lower:
        return [("id", 0x03), ("password", 0xFD), ("secret", 0xFD)], [("1", PASSWORD_SHA1, "")]
    if "select `name` from `players` where `name`" in lower and "`account_id`" in lower:
        return [("name", 0xFD)], [("Fantasy Test",)]
    if "from `ip_bans`" in lower or "from `account_bans`" in lower or "from `player_namelocks`" in lower:
        return [("dummy", 0x03)], []

    if "from `players` as `p` join `accounts` as `a`" in lower:
        return [
            ("id", 0x03), ("account_id", 0x03), ("group_id", 0x03),
            ("type", 0x03), ("premium_ends_at", 0x08),
        ], [("1", "1", "1", "1", "0")]

    if "from `players` where `id` = 1" in lower and "skill_fishing_tries" in lower:
        columns = [
            "id", "name", "account_id", "group_id", "sex", "vocation", "experience", "level",
            "maglevel", "health", "healthmax", "blessings", "mana", "manamax", "manaspent", "soul",
            "lookbody", "lookfeet", "lookhead", "looklegs", "looktype", "lookaddons", "posx", "posy",
            "posz", "cap", "lastlogin", "lastlogout", "lastip", "conditions", "skulltime", "skull",
            "town_id", "balance", "offlinetraining_time", "offlinetraining_skill", "stamina", "skill_fist",
            "skill_fist_tries", "skill_club", "skill_club_tries", "skill_sword", "skill_sword_tries",
            "skill_axe", "skill_axe_tries", "skill_dist", "skill_dist_tries", "skill_shielding",
            "skill_shielding_tries", "skill_fishing", "skill_fishing_tries", "direction",
        ]
        values: list[object] = [
            "1", "Fantasy Test", "1", "1", "1", "0", "4200", "8", "0", "150", "150", "0",
            "0", "0", "0", "0", "0", "0", "0", "0", "136", "0", "95", "117", "7", "400",
            "0", "0", "0", b"", "0", "0", "1", "0", "43200", "-1", "2520", "10", "0", "10",
            "0", "10", "0", "10", "0", "10", "0", "10", "0", "10", "0", "2",
        ]
        return [(column, 0xFD) for column in columns], [tuple(values)]

    if "select `id`, `name`, `password`, `type`, `premium_ends_at` from `accounts` where `id`" in lower:
        return [
            ("id", 0x03), ("name", 0xFD), ("password", 0xFD), ("type", 0x03), ("premium_ends_at", 0x08),
        ], [("1", "fantasy", PASSWORD_SHA1, "1", "0")]

    if "select `save` from `players`" in lower:
        return [("save", 0x03)], [("1",)]

    optional_tables = [
        "guild_membership", "guild_wars", "player_spells", "player_items", "player_depotitems",
        "player_inboxitems", "player_storeinboxitems", "player_storage", "account_viplist", "player_deaths",
        "market_offers", "house_lists", "houses", "players_online", "account_storage",
    ]
    if lower.startswith("select") and any(table in lower for table in optional_tables):
        return guessed_columns(normalized), []

    if lower.startswith("select count("):
        alias = "COUNT(*)"
        match = re.search(r" as `?([a-zA-Z0-9_]+)`?", normalized, re.IGNORECASE)
        if match:
            alias = match.group(1)
        return [(alias, 0x08)], [("0",)]

    if lower.startswith("select"):
        return guessed_columns(normalized), []

    return None


class FixtureServer:
    def __init__(self, host: str, port: int, log_path: Path) -> None:
        self.host = host
        self.port = port
        self.log_path = log_path
        self._log_lock = threading.Lock()

    def log_query(self, query: str) -> None:
        with self._log_lock:
            with self.log_path.open("a", encoding="utf-8") as stream:
                stream.write(query.replace("\n", " ") + "\n")

    def handle(self, conn: socket.socket) -> None:
        try:
            salt1 = b"12345678"
            salt2 = b"abcdefghijkl"
            capabilities = 0x00000001 | 0x00000004 | 0x00000008 | 0x00000200 | 0x00002000 | 0x00008000 | 0x00080000
            handshake = (
                b"\x0A" + b"5.7.0-fantasy-fixture\x00" + struct.pack("<I", 1234) + salt1 + b"\x00" +
                struct.pack("<H", capabilities & 0xFFFF) + b"\x21" + struct.pack("<H", 2) +
                struct.pack("<H", (capabilities >> 16) & 0xFFFF) + b"\x15" + b"\x00" * 10 +
                salt2 + b"\x00" + b"mysql_native_password\x00"
            )
            conn.sendall(packet(handshake, 0))
            header = conn.recv(4)
            if not header:
                return
            length = header[0] | (header[1] << 8) | (header[2] << 16)
            data = b""
            while len(data) < length:
                chunk = conn.recv(length - len(data))
                if not chunk:
                    return
                data += chunk
            conn.sendall(packet(ok_packet(), 2))

            while True:
                header = conn.recv(4)
                if not header:
                    break
                length = header[0] | (header[1] << 8) | (header[2] << 16)
                data = b""
                while len(data) < length:
                    chunk = conn.recv(length - len(data))
                    if not chunk:
                        break
                    data += chunk
                if not data:
                    break
                command = data[0]
                if command == 0x01:
                    break
                if command == 0x02:
                    conn.sendall(packet(ok_packet(), 1))
                    continue
                if command == 0x03:
                    query = data[1:].decode("utf-8", "replace")
                    self.log_query(query)
                    response = query_response(query)
                    if response is None:
                        conn.sendall(packet(ok_packet(), 1))
                    else:
                        send_resultset(conn, *response)
                    continue
                conn.sendall(packet(ok_packet(), 1))
        finally:
            conn.close()

    def serve(self) -> None:
        self.log_path.parent.mkdir(parents=True, exist_ok=True)
        self.log_path.write_text("", encoding="utf-8")
        with socket.socket() as listener:
            listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            listener.bind((self.host, self.port))
            listener.listen(50)
            print(f"TFS1098_MYSQL_FIXTURE READY {self.host}:{self.port}", flush=True)
            while True:
                conn, _ = listener.accept()
                threading.Thread(target=self.handle, args=(conn,), daemon=True).start()


def main() -> int:
    parser = argparse.ArgumentParser(description="Deterministic MySQL-protocol fixture for TFS1098 homologation")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=3306)
    parser.add_argument("--log", default="tfs1098-mysql-fixture.log")
    args = parser.parse_args()
    FixtureServer(args.host, args.port, Path(args.log).resolve()).serve()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
