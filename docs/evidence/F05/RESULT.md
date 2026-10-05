# F05 Evidence — Fantasy Protocol v1 + First Native Play

Status: **IN_PROGRESS / PROTOCOL CODEC SLICE IMPLEMENTED**

## Protocol contract frozen for the first native play

`Shared/Protocol/protocol-v1.yaml` now fixes the v1 TCP framing contract:

- 16-byte little-endian envelope;
- magic `FNTY`;
- protocol version `uint16`;
- message type `uint16`;
- payload length `uint32` excluding the envelope;
- sequence `uint32`;
- maximum payload 4 MiB;
- UTF-8 strings and variable-length fields prefixed by `uint32`.

Movement is now explicitly authoritative:

```text
Client -> MoveRequest(direction)
Server -> EntityMove(entityId, x, y, z, direction)
```

The client no longer has any protocol message that can authoritatively set its absolute position.

ADR-014 freezes these rules for protocol v1.

## Shared C++ codec

`Shared/Protocol/FantasyProtocol.*` implements the first binary codec slice:

- envelope encode/decode;
- frame-size inspection suitable for TCP stream buffering;
- protocol/magic/message-type validation;
- maximum payload validation;
- little-endian serialization;
- `Hello` / `HelloAck`;
- `LoginDev` / `LoginOk`;
- `EnterWorld`;
- `MoveRequest`;
- authoritative `EntityMove`.

`Shared/Protocol/FantasyProtocolTests.cpp` covers framing bytes, little-endian ordering, partial envelope behavior, bad magic/version/type rejection, truncation, payload limits, message roundtrips and invalid movement direction rejection.

## Next F05 slice

After the codec CI gate is green:

1. add typed `MapChunk`, entity add/remove, error and disconnect codecs;
2. create a pure server session state machine for Hello → LoginDev → EnterWorld → MoveRequest without sockets;
3. connect that state machine to the F04 `WorldRuntime`;
4. add TCP transport;
5. create the minimum Fantasy Client and prove first native play.

No OTBM or protocol 10.98 is used by this F05 path.
