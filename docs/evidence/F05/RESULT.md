# F05 Evidence — Fantasy Protocol v1 + First Native Play

Status: **PASS — REAL WINDOWS FIRST PLAY, RECONNECT AND FINAL REGRESSION COMPLETE**

F05 is formally closed after C00–C07 in `docs/CODEX-F05-WINDOWS.md` passed on
a real Windows machine, including interactive Studio/Client, authoritative
movement/rejection, reconnect, cleanup and final regression. F06 remains
NOT STARTED; this closeout adds evidence/documentation only.

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

## Real Windows closeout — 2026-10-05

Tested official baseline: `621ca7b499330b61f653345590f1897c3b1f76d2`.
Windows 11 Home 10.0.26200; Intel HD Graphics 5500; Release;
SDL3 + SDL_Renderer3 + Dear ImGui (ADR-019).

- C01/C07 full native builds: PASS, validators and 13 CTests (Studio 5,
  Server 6, Client 2), including native TCP integration.
- C02 headless sessions: PASS on 17171 and 17172, spawn 100,100,7,
  4 chunks / 64 tiles, East 101,100,7, clean Disconnect and exit 0.
- C03 real Studio: PASS, all requested operations and Save/Reopen;
  real screenshots in F03 evidence; canonical FMAP restored.
- C04 real GUI on 17173: PASS, map/object/player markers and authoritative
  coordinates. Physical arrows were operator-confirmed: East 101,100,7,
  West 99,100,7, South 100,101,7, North 100,99,7 from the spawn area.
  Codex captured the live East state and bounds rejection at 103,100,7 with
  Server error 2001; position remained on the last valid tile.
- C05 new visual session on 17174: PASS, reset to spawn, map received again,
  physical Right produced 101,100,7, captured in the live GUI.
- C04/C05 closing the GUI: PASS; clean_disconnect=true, state=STOPPED,
  Client/Server and scripts exit 0; no orphan processes/active session sockets.
- C07 headless final session on 17175: PASS, spawn to East and clean shutdown;
  unchanged FMAP/GUI sources, git diff --check passed, zero Fantasy processes.

Real screenshots: [C04 initial](windows/c04-renderer3-initial.jpg),
[C04 East](windows/c04-renderer3-east-confirmed.jpg),
[C04 rejected movement](windows/c04-renderer3-rejected.jpg),
[C05 reconnect initial](windows/c05-renderer3-initial.jpg),
[C05 reconnect East](windows/c05-renderer3-east.jpg).

All listeners stayed 127.0.0.1-only. No legacy runtime/format participated.
Computer Use arrow injection translated Right to Keypad 6; physical arrows
were used instead. Temporary diagnostics/capture-option trials were removed;
no GUI/core/gameplay/protocol/network/FMAP code was changed for closeout.
Full evidence, preserved startup history and backup branches are recorded in
[WINDOWS-VALIDATION.md](WINDOWS-VALIDATION.md).

Published closeout SHA: `c414819d5c98034dcef1f8e666ad6310b3874908`.
Foundation Checks [run 218](https://github.com/vetrasistem-ui/New-fantasy/actions/runs/37369516321)
on that exact SHA remains **PENDING / QUEUED** with no executed steps.
GitHub [reported runner-assignment delays](https://www.githubstatus.com/incidents/3q1yb5m7ltvb)
on 2026-10-05. All local C00–C07 gates passed; remote CI success is not claimed.

## Codex entry point

```text
docs/CODEX-NEXT.md
```

The complete ordered Windows execution contract is:

```text
docs/CODEX-F05-WINDOWS.md
```
