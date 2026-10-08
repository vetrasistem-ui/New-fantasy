# Vanilla TFS 1.4.2 / 10.98 Runtime Homologation

**Status:** PASS for Fantasy OTBM load, TFS startup, 10.98 login, character entry and movement.

## Target runtime

The validation used the official upstream release package:

- project: `otland/forgottenserver`;
- release: `v1.4.2`;
- Git SHA: `31d6e85d`;
- official artifact: `tfs-v1.4.2-ubuntu-gcc.tar.gz`;
- protocol line: 10.98.

The package was downloaded from the official GitHub release by the repository workflow `TFS 1.4.2 Homologation Tool`. The TFS executable was not rebuilt or patched.

## Fantasy output under test

The tested map was the OTBM written by Fantasy from the pinned real 10.98 `global_dash` project:

```text
source canonical tiles: 6,106,271
writer items:           6,499,901
writer tile areas:      277
output bytes:            64,919,912
SHA-256: ae640e8b3a27b8ef108de56673c5b991ac99de5d6cfacb7516479ea2cbf74bed
```

Runtime companions were the pinned 10.98 `items.otb`, `map-house.xml` and `map-spawn.xml`. The source OTBM was never overwritten.

## Database fixture boundary

The execution container did not provide MariaDB/MySQL. TFS requires a MySQL connection before map and login services become available, so homologation used a minimal MySQL-protocol fixture.

For the map-load gate it returned startup/config responses and empty persistence results. For the login gate it additionally exposed one deterministic account/player fixture:

```text
account:   fantasy
character: Fantasy Test
position:  714,787,7
```

The fixture does not parse OTBM/OTB, create tiles, move creatures or implement the Tibia protocol. Those paths are executed by the unmodified TFS 1.4.2 binary. This proves runtime/protocol compatibility, not production database persistence behavior.

## Vanilla TFS OTBM result — PASS

Relevant upstream output:

```text
The Forgotten Server - Version v1.4.2
Git SHA1 31d6e85d dated 2022-05-08T20:27:14-04:00

>> Loading items
>> Loading map
> Map size: 30000x30000.
> Map loading time: 2.757 seconds.
...
>> Loaded all modules, server starting up...
>> Forgotten Server Online!
```

There were no `IOMap`/OTBM format rejection messages. Once `map-house.xml` and `map-spawn.xml` were placed under the exact names preserved by the OTBM metadata, there were no auxiliary-file-not-found errors.

## Expected PokéTibia content warnings

The real project is a PokéTibia map while the vanilla TFS datapack does not define its Pokémon/NPC content. Vanilla therefore emits expected warnings such as:

```text
[Warning - Spawn::addMonster] Can not find Charizard
[Warning - Spawn::addMonster] Can not find Dragonite
[Warning - Spawn::addMonster] Can not find Lucario
```

These are datapack content gaps, not Writer/OTBM failures. They also prove TFS accepted the OTBM and proceeded to parse the external spawn definitions.

## 10.98 login server — PASS

A headless 10.98 compatibility client reproduced the normal protocol stack used by OTClient/Tibia 10.x:

- 10.98 login packet;
- RSA login blocks using the runtime public key;
- Adler checksum framing;
- XTEA session encryption;
- account/password authentication;
- session-key response;
- character-list response.

Observed result:

```text
MOTD 31
Welcome to The Forgotten Server!

SESSION_KEY 'fantasy\nfantasy\n\n...'
CHAR_LIST [('Fantasy Test', ('Forgotten', '127.0.0.1', 7172, 0))]
LOGIN_SERVER_PASS
```

## 10.98 game entry — PASS

The same client connected to the game service on port 7172, parsed the TFS challenge (`0x1F`), sent `ClientPendingGame` with the session key, completed RSA/XTEA negotiation and received the encrypted game startup payload.

Observed startup packet began with normal 10.98 login-success/game data:

```text
GAME_CHALLENGE <timestamp> <random>
GAME_PAYLOAD 13130 bytes
first opcode: 0x17 (GameServerLoginSuccess)
```

The player was loaded at the real exported-map position:

```text
714,787,7
```

## 10.98 movement — PASS

The compatibility client sent the normal north-walk opcode:

```text
ClientWalkNorth = 0x65
```

TFS responded with:

```text
GameServerMoveCreature = 0x6D
```

The response decodes to the exact coordinate transition:

```text
before: 714,787,7
after:  714,786,7
```

The same response also included the expected map-row update following the movement packet.

This proves that the unmodified TFS 1.4.2 runtime accepted the Fantasy-written map, authenticated a 10.98 client, instantiated a player on that map and executed movement over it.

## Gate conclusion

```text
Fantasy canonical MapDocument          PASS
        ↓
Fantasy OTBM v3 Writer                 PASS
        ↓
Fantasy semantic reopen                PASS
        ↓
vanilla TFS 1.4.2 IOMap load           PASS
        ↓
TFS server startup / Online             PASS
        ↓
10.98 login + session + char list       PASS
        ↓
10.98 enter exported map                PASS
        ↓
10.98 walk 714,787,7 → 714,786,7        PASS
```

## Remaining integration work

The compatibility foundation is proven. Remaining work is product/runtime integration rather than proving the basic file/protocol boundary:

1. wire the proven export/start/stop flow into `Tfs1098 RuntimeBackend`;
2. use a real MariaDB fixture for persistence lifecycle tests;
3. repeat login/movement with the selected full Windows/OTClient 10.98 UI build;
4. surface runtime logs/status inside Fantasy Studio;
5. package server/client deployment workflow for local/VPS use.
