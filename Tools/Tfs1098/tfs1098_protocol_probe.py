#!/usr/bin/env python3
"""Minimal protocol 10.98 probe for the official TFS 1.4.2 runtime.

This is intentionally a homologation tool, not a game client. It exercises the
normal TFS gameworld handshake using the server challenge, RSA_NO_PADDING,
Adler32, XTEA, a real account session key and an explicit logout packet.
"""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import socket
import struct
import subprocess
import time
import zlib

MASK32 = 0xFFFFFFFF
XTEA_DELTA = 0x9E3779B9
PROTOCOL_VERSION = 1098
CLIENT_VERSION = 1098
CLIENT_PENDING_GAME = 0x0A
CLIENT_LOGOUT = 0x14
SERVER_CHALLENGE = 0x1F
SERVER_LOGIN_SUCCESS = 0x17
SERVER_ERROR = 0x14

# Deterministic key: this is a test client and the key is ephemeral transport
# material, not a credential.
XTEA_KEY = (0x12345678, 0x9ABCDEF0, 0x0F1E2D3C, 0x4B5A6978)


def u16(value: int) -> bytes:
    return struct.pack("<H", value)


def u32(value: int) -> bytes:
    return struct.pack("<I", value)


def add_string(value: str) -> bytes:
    data = value.encode("utf-8")
    if len(data) > 0xFFFF:
        raise ValueError("protocol string exceeds uint16 length")
    return u16(len(data)) + data


def recv_exact(sock: socket.socket, size: int) -> bytes:
    chunks: list[bytes] = []
    remaining = size
    while remaining:
        chunk = sock.recv(remaining)
        if not chunk:
            raise ConnectionError(f"connection closed with {remaining} bytes remaining")
        chunks.append(chunk)
        remaining -= len(chunk)
    return b"".join(chunks)


def recv_wire(sock: socket.socket) -> bytes:
    size = struct.unpack("<H", recv_exact(sock, 2))[0]
    if size <= 0:
        raise ValueError("invalid zero-length network packet")
    return recv_exact(sock, size)


def frame_checksum(payload: bytes) -> bytes:
    checksum = zlib.adler32(payload) & MASK32
    framed = u32(checksum) + payload
    return u16(len(framed)) + framed


def validate_checksum(packet: bytes) -> bytes:
    if len(packet) < 4:
        raise ValueError("packet is too short for Adler32")
    expected = struct.unpack_from("<I", packet, 0)[0]
    payload = packet[4:]
    actual = zlib.adler32(payload) & MASK32
    if actual != expected:
        raise ValueError(f"Adler32 mismatch: got 0x{expected:08x}, expected 0x{actual:08x}")
    return payload


def xtea_encrypt(data: bytes, key: tuple[int, int, int, int]) -> bytes:
    buffer = bytearray(data)
    if len(buffer) % 8:
        buffer.extend(b"\0" * (8 - (len(buffer) % 8)))

    for offset in range(0, len(buffer), 8):
        v0, v1 = struct.unpack_from("<II", buffer, offset)
        total = 0
        for _ in range(32):
            v0 = (
                v0
                + (((((v1 << 4) & MASK32) ^ (v1 >> 5)) + v1) ^ (total + key[total & 3]))
            ) & MASK32
            total = (total + XTEA_DELTA) & MASK32
            v1 = (
                v1
                + (((((v0 << 4) & MASK32) ^ (v0 >> 5)) + v0) ^ (total + key[(total >> 11) & 3]))
            ) & MASK32
        struct.pack_into("<II", buffer, offset, v0, v1)
    return bytes(buffer)


def xtea_decrypt(data: bytes, key: tuple[int, int, int, int]) -> bytes:
    if len(data) % 8:
        raise ValueError("encrypted payload is not an 8-byte multiple")
    buffer = bytearray(data)

    for offset in range(0, len(buffer), 8):
        v0, v1 = struct.unpack_from("<II", buffer, offset)
        total = (XTEA_DELTA * 32) & MASK32
        for _ in range(32):
            v1 = (
                v1
                - (((((v0 << 4) & MASK32) ^ (v0 >> 5)) + v0) ^ (total + key[(total >> 11) & 3]))
            ) & MASK32
            total = (total - XTEA_DELTA) & MASK32
            v0 = (
                v0
                - (((((v1 << 4) & MASK32) ^ (v1 >> 5)) + v1) ^ (total + key[total & 3]))
            ) & MASK32
        struct.pack_into("<II", buffer, offset, v0, v1)
    return bytes(buffer)


def rsa_encrypt_no_padding(block: bytes, key_path: Path) -> bytes:
    if len(block) != 128:
        raise ValueError(f"RSA block must be exactly 128 bytes, got {len(block)}")
    result = subprocess.run(
        [
            "openssl",
            "pkeyutl",
            "-encrypt",
            "-inkey",
            str(key_path),
            "-pkeyopt",
            "rsa_padding_mode:none",
        ],
        input=block,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )
    if result.returncode != 0:
        raise RuntimeError("OpenSSL RSA encryption failed: " + result.stderr.decode("utf-8", "replace"))
    if len(result.stdout) != 128:
        raise RuntimeError(f"OpenSSL returned unexpected RSA size {len(result.stdout)}")
    return result.stdout


def parse_challenge(packet: bytes) -> tuple[int, int]:
    payload = validate_checksum(packet)
    if len(payload) < 8:
        raise ValueError("challenge payload is truncated")
    inner_length = struct.unpack_from("<H", payload, 0)[0]
    if inner_length != 6:
        raise ValueError(f"unexpected challenge inner length {inner_length}")
    if payload[2] != SERVER_CHALLENGE:
        raise ValueError(f"unexpected challenge opcode 0x{payload[2]:02x}")
    timestamp = struct.unpack_from("<I", payload, 3)[0]
    random_byte = payload[7]
    return timestamp, random_byte


def make_game_login(
    key_path: Path,
    timestamp: int,
    random_byte: int,
    account: str,
    password: str,
    character: str,
) -> bytes:
    session_key = f"{account}\n{password}\n\n0"

    rsa_plain = bytearray()
    rsa_plain.append(0)
    for part in XTEA_KEY:
        rsa_plain.extend(u32(part))
    rsa_plain.append(0)  # gamemaster flag
    rsa_plain.extend(add_string(session_key))
    rsa_plain.extend(add_string(character))
    rsa_plain.extend(u32(timestamp))
    rsa_plain.append(random_byte)
    if len(rsa_plain) > 128:
        raise ValueError("game login RSA payload exceeds 128 bytes")
    rsa_plain.extend(b"\0" * (128 - len(rsa_plain)))

    encrypted_rsa = rsa_encrypt_no_padding(bytes(rsa_plain), key_path)

    # Connection::parsePacket consumes this protocol ID before
    # ProtocolGame::onRecvFirstMessage. The next seven bytes match the
    # 10.98 ClientVersion + ContentRevision + PreviewState feature layout.
    body = bytearray()
    body.append(CLIENT_PENDING_GAME)
    body.extend(u16(1))  # CLIENTOS_LINUX; avoids OTClient-only extended-opcode greeting
    body.extend(u16(PROTOCOL_VERSION))
    body.extend(u32(CLIENT_VERSION))
    body.extend(u16(0))  # content revision: not validated by vanilla TFS
    body.append(0)       # preview state/client type byte
    body.extend(encrypted_rsa)
    return frame_checksum(bytes(body))


def make_encrypted_packet(body: bytes) -> bytes:
    inner = u16(len(body)) + body
    encrypted = xtea_encrypt(inner, XTEA_KEY)
    return frame_checksum(encrypted)


def decrypt_server_packet(packet: bytes) -> bytes:
    encrypted = validate_checksum(packet)
    decrypted = xtea_decrypt(encrypted, XTEA_KEY)
    if len(decrypted) < 2:
        raise ValueError("decrypted packet is too short")
    size = struct.unpack_from("<H", decrypted, 0)[0]
    if size + 2 > len(decrypted):
        raise ValueError(f"invalid decrypted inner size {size} for {len(decrypted)} bytes")
    return decrypted[2 : 2 + size]


def decode_error(body: bytes) -> str:
    if len(body) < 3:
        return "server returned a truncated error packet"
    length = struct.unpack_from("<H", body, 1)[0]
    return body[3 : 3 + length].decode("utf-8", "replace")


def login_and_logout(args: argparse.Namespace) -> None:
    key_path = Path(args.key).resolve()
    if not key_path.is_file():
        raise FileNotFoundError(f"TFS RSA key not found: {key_path}")

    with socket.create_connection((args.host, args.port), timeout=args.timeout) as sock:
        sock.settimeout(args.timeout)
        challenge = recv_wire(sock)
        timestamp, random_byte = parse_challenge(challenge)
        print(f"GAME_CHALLENGE timestamp={timestamp} random={random_byte}", flush=True)

        sock.sendall(
            make_game_login(
                key_path,
                timestamp,
                random_byte,
                args.account,
                args.password,
                args.character,
            )
        )

        logged_in = False
        for _ in range(12):
            body = decrypt_server_packet(recv_wire(sock))
            if not body:
                continue
            opcode = body[0]
            print(f"GAME_PACKET opcode=0x{opcode:02x} bytes={len(body)}", flush=True)
            if opcode == SERVER_ERROR:
                raise RuntimeError("TFS login rejected: " + decode_error(body))
            if opcode == SERVER_LOGIN_SUCCESS:
                logged_in = True
                break

        if not logged_in:
            raise RuntimeError("TFS did not emit GameServerLoginSuccess (0x17)")

        print("GAME_LOGIN PASS", flush=True)
        if args.ready_file:
            ready_path = Path(args.ready_file)
            ready_path.parent.mkdir(parents=True, exist_ok=True)
            ready_path.write_text("online\n", encoding="utf-8")

        if args.hold_seconds > 0:
            time.sleep(args.hold_seconds)

        sock.sendall(make_encrypted_packet(bytes([CLIENT_LOGOUT])))

        # A clean logout normally closes the connection. Do not treat a timeout
        # as success: it could mean the player was on a no-logout tile.
        deadline = time.monotonic() + args.timeout
        while time.monotonic() < deadline:
            try:
                data = sock.recv(4096)
            except socket.timeout:
                break
            if not data:
                print("GAME_LOGOUT PASS", flush=True)
                return

        raise RuntimeError("TFS did not close the game connection after ClientLogout")


def main() -> int:
    parser = argparse.ArgumentParser(description="TFS 1.4.2 protocol 10.98 login/logout probe")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=7172)
    parser.add_argument("--key", required=True, help="path to the TFS RSA private key.pem")
    parser.add_argument("--account", default="fantasy")
    parser.add_argument("--password", default="fantasy")
    parser.add_argument("--character", default="Fantasy Test")
    parser.add_argument("--ready-file", default="")
    parser.add_argument("--hold-seconds", type=float, default=0.0)
    parser.add_argument("--timeout", type=float, default=10.0)
    args = parser.parse_args()

    login_and_logout(args)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
