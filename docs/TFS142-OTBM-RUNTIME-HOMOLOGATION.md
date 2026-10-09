# Vanilla TFS 1.4.2 / 10.98 Runtime Homologation

**Status:** PASS for Fantasy OTBM load, TFS startup, protocol 10.98 login/game entry/movement, real MariaDB bootstrap and save/restart/relogin persistence.

## Target runtime

The validation uses the official upstream release package:

- project: `otland/forgottenserver`;
- release: `v1.4.2`;
- Git SHA: `31d6e85d`;
- official artifact: `tfs-v1.4.2-ubuntu-gcc.tar.gz`;
- protocol line: 10.98.

The TFS executable is not rebuilt or patched for these homologation gates.

## Fantasy output under test

The real-map gate used the OTBM written by Fantasy from the pinned 10.98 `global_dash` project:

```text
source canonical tiles: 6,106,271
writer items:           6,499,901
writer tile areas:      277
output bytes:            64,919,912
SHA-256: ae640e8b3a27b8ef108de56673c5b991ac99de5d6cfacb7516479ea2cbf74bed
```

Runtime companions were the matching 10.98 `items.otb`, `map-house.xml` and `map-spawn.xml`. The source OTBM was never overwritten.

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

## 10.98 protocol gate — PASS

The initial protocol homologation used a deterministic MySQL-protocol fixture only for persistence rows. The unmodified TFS executable still performed RSA, Adler, XTEA, authentication, player placement and movement.

Observed login-server result:

```text
MOTD 31
SESSION_KEY received
CHAR_LIST [('Fantasy Test', ('Forgotten', '127.0.0.1', 7172, 0))]
LOGIN_SERVER_PASS
```

Observed gameworld result:

```text
GAME_CHALLENGE received
GAME_PAYLOAD 13130 bytes
first opcode: 0x17 (GameServerLoginSuccess)
```

Movement proof:

```text
ClientWalkNorth = 0x65
GameServerMoveCreature = 0x6D
714,787,7 -> 714,786,7
```

This proves the basic Fantasy-written-map / vanilla-TFS / 10.98 protocol boundary independently from production persistence.

## Real MariaDB bootstrap — PASS

A second gate replaced the database fixture with a real `mariadb:10.11` service and the official TFS v1.4.2 `schema.sql`.

Workflow:

```text
.github/workflows/tfs142-mariadb-homologation.yml
```

The workflow is now `workflow_dispatch` only. It was temporarily triggered through the PR solely to collect the initial homologation evidence, then returned to manual-only mode.

Bootstrap procedure:

1. start MariaDB 10.11;
2. import official `schema.sql`;
3. seed account `fantasy` and player `Fantasy Test`;
4. insert a stale `players_online` row;
5. start the official TFS 1.4.2 binary;
6. require `Forgotten Server Online!`;
7. require the stale online row to be cleared by TFS;
8. verify the database migration executed by TFS.

Observed result:

```text
players_online_after_start=0
db_version_after_start=30
TFS142_MARIADB_BOOTSTRAP PASS

The Forgotten Server - Version v1.4.2
> Updating database to version 29 (account storages)
> Database has been updated to version 30.
>> Loading map
> Map size: 2048x2048.
>> Forgotten Server Online!
```

The official schema begins at database version 29 for this package and the runtime itself performs the `29 -> 30` migration during startup. That migration is part of the PASS evidence, not a mismatch.

## Real MariaDB 10.98 save/restart/relogin — PASS

A reusable homologation probe now lives at:

```text
Tools/Tfs1098/tfs1098_protocol_probe.py
```

It is intentionally a minimal compatibility probe rather than a game client. It uses the normal TFS gameworld protocol boundary:

- server challenge `0x1F`;
- protocol/client version 1098;
- RSA login block;
- Adler framing;
- XTEA encryption;
- real account/password authentication against MariaDB;
- `GameServerLoginSuccess` (`0x17`);
- `ClientLogout` (`0x14`).

The persistence workflow performed two independent player sessions separated by a full TFS process restart.

Initial database state:

```text
initial_state=100,100,7|0|0
```

Cycle 1:

```text
cycle1_players_online_during_session=1
GAME_CHALLENGE timestamp=1791544061 random=10
GAME_PACKET opcode=0x17 bytes=11378
GAME_LOGIN PASS
GAME_LOGOUT PASS
cycle1_players_online_after_logout=0
state_after_cycle1=95,117,7|1791544061|1791544065
```

TFS then stopped completely and was started again against the same MariaDB database.

Cycle 2:

```text
cycle2_players_online_during_session=1
GAME_CHALLENGE timestamp=1791544071 random=91
GAME_PACKET opcode=0x17 bytes=11078
GAME_LOGIN PASS
GAME_LOGOUT PASS
cycle2_players_online_after_logout=0
state_after_cycle2=95,117,7|1791544071|1791544075
```

Final gate:

```text
TFS1098_MARIADB_PERSISTENCE PASS
persisted_position=95,117,7
lastlogin=1791544061->1791544071
lastlogout=1791544065->1791544075
```

The seeded position `100,100,7` was not a valid player placement for the vanilla map. TFS placed the character at a valid runtime location `95,117,7`, saved that position on logout, and after a complete process restart the second session reused the same persisted location. The database also recorded distinct login/logout timestamps and `players_online` transitioned `0 -> 1 -> 0` in both sessions.

Therefore this gate proves real persistence through the official TFS + MariaDB path rather than a mocked query layer.

## Gate conclusion

```text
Fantasy canonical MapDocument                 PASS
        ↓
Fantasy OTBM v3 Writer                        PASS
        ↓
Fantasy semantic reopen                       PASS
        ↓
vanilla TFS 1.4.2 IOMap load                  PASS
        ↓
TFS server startup / Online                    PASS
        ↓
10.98 login / game entry / movement            PASS
        ↓
real MariaDB 10.11 + official schema           PASS
        ↓
TFS schema migration 29 -> 30                  PASS
        ↓
real account/player login                      PASS
        ↓
players_online lifecycle                       PASS
        ↓
logout/save -> TFS restart -> relogin          PASS
        ↓
persisted position + login/logout timestamps   PASS
```

## Remaining integration work

The file, runtime, protocol and persistence foundations are now proven. Remaining work is primarily product acceptance and higher-level runtime integration:

1. selected full Windows/OTClient 10.98 visual acceptance;
2. expose/manage database/runtime configuration cleanly through the Studio/VPS deployment path;
3. package server/client deployment for local and 24/7 VPS operation;
4. begin mapping advanced PokéTibia runtime capabilities into Fantasy-owned generic systems without copying the reference fork into Fantasy Core.
