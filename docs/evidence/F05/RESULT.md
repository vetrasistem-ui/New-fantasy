# F05 Evidence — Fantasy Protocol v1 + First Native Play

Status: **AUTOMATED TECHNICAL PASS / INTERACTIVE WINDOWS CLOSEOUT PENDING**

F05 is **not yet formally closed**. Repository-side implementation and automated Windows CI are working, including real loopback TCP and a two-process first-play. The remaining gates are interactive Windows validation/evidence described in `docs/CODEX-F05-WINDOWS.md`.

## Native path under test

```text
Fantasy Client
   ↓
Shared TCP transport / FrameStream
   ↓
Fantasy Protocol v1
   ↓
Fantasy Server
   ↓
DevelopmentSession
   ↓
WorldRuntime
   ↓
FMAP
```

No OTBM, TFS runtime, protocol 10.98, DAT/SPR/OTB or OTClient participates in this path.

## Protocol contract

`Shared/Protocol/protocol-v1.yaml` freezes the first-play v1 contract:

- transport: TCP;
- 16-byte little-endian `FNTY` envelope;
- protocol version `uint16`;
- message type `uint16`;
- payload length `uint32` excluding envelope;
- sequence `uint32`;
- maximum payload 4 MiB;
- UTF-8 variable fields prefixed by `uint32`;
- `MapChunk` semantic payload: FMCP v1.

Movement remains explicitly authoritative:

```text
Client -> MoveRequest(direction)
Server -> WorldRuntime validation
Server -> EntityMove(entityId, x, y, z, direction)
```

The Client has no message that can authoritatively set absolute position.

## Shared C++ protocol codec

`Shared/Protocol/FantasyProtocol.*` implements encode/decode and validation for:

- `Hello` / `HelloAck`;
- `LoginDev` / `LoginOk`;
- `EnterWorld`;
- `MapChunk`;
- `MoveRequest`;
- `EntityAdd` / `EntityMove` / `EntityRemove`;
- `Error`;
- `Disconnect`.

Automated tests cover framing bytes, endian order, partial envelope, bad magic/version/type, truncation, payload limit, typed roundtrips, trailing bytes and invalid movement direction.

## FMCP v1 — semantic map payload

`Shared/Protocol/MapChunkPayload.*` encodes one FMAP chunk without falling back to legacy item IDs.

The outer `MapChunk` carries:

```text
regionId
regionOriginX
regionOriginY
chunkX
chunkY
floor
revision
payload
```

FMCP v1 carries each tile's:

```text
tile-local x/y
ground semantic key
object semantic keys
tags
```

The Client reconstructs global coordinates using:

```text
regionOrigin + chunkOffset + tileLocal
```

The development fixture streams 4 chunks / 64 semantic tiles.

## Authoritative Server session

`Server/Network/DevelopmentSession.*` implements:

```text
AwaitHello
   ↓ Hello
AwaitLogin
   ↓ LoginDev
InWorld
   ↓ MoveRequest
WorldRuntime validates
   ↓
EntityMove authoritative reply
```

It also:

- rejects unsupported protocol versions;
- enforces monotonically increasing client sequence numbers;
- spawns the development player at the FMAP development spawn;
- sends `LoginOk + EnterWorld + MapChunk(s) + EntityAdd`;
- removes the entity on clean Disconnect;
- rolls back login state if snapshot/login construction fails.

## Real TCP transport

`Shared/Network/` provides the shared TCP/FrameStream layer used by Server and Client.

F05 listener behavior is intentionally limited:

- loopback only (`127.0.0.1`);
- one development connection in `--serve-once` mode;
- `LoginDev` only;
- not public authentication and not the final 24/7 server mode.

`Server/Network/TcpDevelopmentConnection.*` cleans up the server entity on both clean and abrupt transport disconnects.

## Native Fantasy Client

`Client/Core/DevelopmentClient.*` performs:

- handshake;
- development login;
- EnterWorld;
- MapChunk/FMCP reconstruction;
- EntityAdd validation;
- MoveRequest;
- authoritative EntityMove application;
- clean Disconnect.

Two executables exist:

```text
fantasy-client.exe      # headless / automation
fantasy-client-gui.exe  # first visual native-play gate
```

The visual Client renders semantic tiles using deterministic placeholder colors and sends movement only through the same Client core.

## Integration coverage

`Tests/Integration/NativePlayTests.cpp` proves through real loopback TCP:

- handshake;
- login;
- spawn at `100,100,7`;
- 4 chunks / 64 tiles reconstructed;
- region-origin coordinate resolution;
- authoritative movement;
- clean Disconnect cleanup;
- second independent connection/reconnect;
- abrupt TCP disconnect cleanup.

The workflow also executes `scripts/run-native-play.ps1`, which starts the real `fantasy-server.exe --serve-once` and `fantasy-client.exe` as separate processes.

## Automated Windows evidence

Workflow **run 191**, commit `1b9257f3fdcc186e235380f7a2c1bc7671fd227d`, completed successfully on Windows after fixing the WinSock `min/max` macro conflict.

That run passed:

- layout/contracts;
- Fantasy Project v2;
- FMAP v0;
- FMAP multi-chunk validation;
- Fantasy Protocol/FМCP validation;
- relocation test;
- Studio configure/build/all CTests;
- Server configure/build/all CTests;
- `fantasy-native-play-tests`;
- Client configure/build/CTest;
- real two-process native play;
- Windows artifact packaging for Studio and native runtime.

This proves the **automated technical path** on Windows. It does not replace interactive visual evidence.

## Remaining gates before F05 can be marked PASS

Only Windows real-machine closeout remains:

1. execute the ordered handoff in `docs/CODEX-F05-WINDOWS.md`;
2. close the pending F03 visual Studio gate;
3. run `fantasy-client-gui.exe` against the real Server;
4. visually confirm four chunks / 64 tiles and player marker;
5. visually confirm server-authoritative arrow-key movement;
6. repeat visual connection/reconnect;
7. confirm no orphan Server/Client processes;
8. record screenshots and `docs/evidence/F05/WINDOWS-VALIDATION.md`;
9. rerun full regression on the final SHA;
10. only then change F05 to `PASS` and advance priority to F06.

## Codex entry point

```text
docs/CODEX-NEXT.md
```

The complete ordered Windows execution contract is:

```text
docs/CODEX-F05-WINDOWS.md
```
