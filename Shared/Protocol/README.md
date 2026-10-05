# Fantasy Protocol v1

`Shared/Protocol/protocol-v1.yaml` is the canonical semantic specification. The C++ codec in `FantasyProtocol.*` is the first wire implementation of that contract.

## Transport and envelope

Fantasy Protocol v1 runs over TCP and uses a fixed 16-byte little-endian envelope:

```text
0..3   magic            "FNTY"
4..5   protocolVersion  uint16
6..7   messageType      uint16
8..11  payloadLength    uint32
12..15 sequence         uint32
16..   payload
```

`payloadLength` excludes the 16-byte envelope. Payloads are limited to 4 MiB. Variable-length strings/byte arrays use a little-endian `uint32` length prefix; strings are UTF-8.

A TCP receiver must buffer until the complete 16-byte envelope exists, derive the expected total frame size, then wait until that exact number of bytes is available before decoding the frame. TCP packet boundaries are never treated as message boundaries.

## Authority rule

Client messages express **intent**. Server messages publish **authoritative state**.

Movement is deliberately split:

```text
Client -> MoveRequest(direction)
Server validates WorldRuntime
Server -> EntityMove(entityId, x, y, z, direction)
```

The client cannot set its own absolute position. This rule will also apply to future combat, inventory and interaction commands.

## Current codec slice

The shared C++ codec currently covers the v1 envelope plus:

- `Hello` / `HelloAck`;
- `LoginDev` / `LoginOk`;
- `EnterWorld`;
- `MoveRequest`;
- `EntityMove`.

`MapChunk`, entity add/remove, errors and disconnect are already reserved in the YAML contract and will be added to the typed codec as the F05 native session progresses.
