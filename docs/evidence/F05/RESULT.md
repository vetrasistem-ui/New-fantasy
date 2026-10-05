# F05 Evidence — Fantasy Protocol v1 + First Native Play

Status: **IN_PROGRESS / CODEC + AUTHORITATIVE SESSION PASS**

## Protocol contract frozen for the first native play

`Shared/Protocol/protocol-v1.yaml` fixes the v1 TCP framing contract:

- 16-byte little-endian envelope;
- magic `FNTY`;
- protocol version `uint16`;
- message type `uint16`;
- payload length `uint32` excluding the envelope;
- sequence `uint32`;
- maximum payload 4 MiB;
- UTF-8 strings and variable-length fields prefixed by `uint32`.

Movement is explicitly authoritative:

```text
Client -> MoveRequest(direction)
Server validates WorldRuntime
Server -> EntityMove(entityId, x, y, z, direction)
```

The client has no message that can authoritatively set its absolute position. ADR-014 freezes these rules for protocol v1.

## Shared C++ codec

`Shared/Protocol/FantasyProtocol.*` implements:

- envelope encode/decode;
- frame-size inspection for TCP stream buffering;
- protocol/magic/message-type validation;
- maximum payload validation;
- little-endian serialization;
- `Hello` / `HelloAck`;
- `LoginDev` / `LoginOk`;
- `EnterWorld`;
- `MapChunk`;
- `MoveRequest`;
- `EntityAdd` / `EntityMove` / `EntityRemove`;
- `Error`;
- `Disconnect`.

`Shared/Protocol/FantasyProtocolTests.cpp` covers framing bytes, endian ordering, partial envelope behavior, bad magic/version/type rejection, truncation, payload limits, typed message roundtrips, trailing payload rejection and invalid movement direction rejection.

## Authoritative development session

`Server/Network/DevelopmentSession.*` adds a socket-independent state machine above the F04 `WorldRuntime`:

```text
AwaitHello
   ↓ Hello
AwaitLogin
   ↓ LoginDev
InWorld
   ↓ MoveRequest
WorldRuntime validates movement
   ↓
EntityMove authoritative reply
```

The session also validates monotonically increasing client sequence numbers, rejects unsupported protocol versions, creates the player entity at the FMAP development spawn, returns `LoginOk + EnterWorld + EntityAdd`, and removes the entity on clean disconnect.

`Server/Network/DevelopmentSessionTests.cpp` proves Hello/HelloAck, development login, entity creation, authoritative movement, replayed-sequence rejection, disconnect cleanup and requested-protocol mismatch handling.

## Automated evidence

Workflow run **134**, commit `e5b2425a20c4d6c6a4cf4800831933d6136b29a5`, completed successfully on Windows with the protocol/session slice included.

PASS in that run:

- all project/FM﻿AP/protocol validators;
- Studio configure/build/tests;
- Server configure/build;
- F04 world runtime tests;
- Fantasy Protocol codec tests;
- DevelopmentSession tests;
- server smoke test.

## Remaining F05 gates

- define/encode the semantic tile payload carried by `MapChunk`;
- make login send the FMAP chunks needed by the player;
- add TCP stream buffering/transport around the already-tested frame/session layers;
- create the minimal Fantasy Client;
- prove Client → TCP → Server → FMAP world entry and movement;
- validate disconnect/reconnect over the real transport.

No OTBM or protocol 10.98 is used by this F05 path.
