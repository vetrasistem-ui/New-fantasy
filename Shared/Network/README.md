# Shared Network — F05 transport boundary

The first native transport is intentionally small and development-only.

```text
Fantasy Client
    ↓ TCP loopback
FrameStream
    ↓
Fantasy Protocol v1
    ↓
DevelopmentSession
    ↓
Fantasy Server / WorldRuntime / FMAP
```

## Current guarantees

- `TcpTransport` is a blocking IPv4 transport with exact-read/exact-write helpers;
- Windows uses Winsock2, POSIX builds use standard BSD sockets;
- `FrameStream` reconstructs Fantasy Protocol frames from the 16-byte envelope plus declared payload length;
- protocol payload limits are validated before the full frame is accepted;
- the F05 listener binds **127.0.0.1 only**;
- tests use OS-assigned ephemeral loopback ports;
- disconnect/reconnect is covered by the native-play integration test.

## Security boundary

`LoginDev` is intentionally unauthenticated and exists only for the first local native-play gate. **Do not expose the F05 development listener to the public internet or a VPS public interface.**

Before public multiplayer networking, a later phase must add at minimum real account authentication/session tokens, public bind configuration, connection/rate limits, timeouts, abuse controls, production logging/metrics and a deliberate transport-encryption decision.

## Canonical implementation

```text
Shared/Network/TcpTransport.*
Shared/Network/FrameStream.*
Server/Network/TcpDevelopmentConnection.*
Tests/Integration/NativePlayTests.cpp
```
