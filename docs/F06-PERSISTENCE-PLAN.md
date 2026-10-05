# F06 — Persistence / Database Plan

Status: **PREPARED / BLOCKED UNTIL F03+F05 WINDOWS CLOSEOUT PASS**

This document prepares F06 so the next implementation can start in a controlled order after the native visual gates are closed. It is **not permission to start F06 early**.

## Goal

Persist the smallest authoritative game state needed to survive Server restart without coupling gameplay code directly to one database engine.

First F06 milestone:

```text
create/load development account
        ↓
create/load character
        ↓
enter FMAP at saved position
        ↓
move through Fantasy Server
        ↓
save authoritative position
        ↓
stop Server
        ↓
restart Server
        ↓
load same character at saved position
```

No OTBM, TFS database schema, 10.98 account model or OTClient data format may enter this path.

## Scope

F06 includes only:

- persistence abstraction owned by Fantasy Server;
- schema migrations;
- local development account identity;
- character identity and name;
- authoritative character position;
- create/load/save character;
- clean logout save;
- explicit save/checkpoint API;
- restart/reconnect persistence tests;
- deterministic database fixtures for automated tests.

F06 explicitly does **not** include:

- public account registration;
- passwords/plaintext credentials;
- website authentication;
- OAuth/social login;
- inventory/items;
- skills/stats;
- quests;
- houses;
- guilds;
- public VPS networking;
- multi-server sharding.

Those belong to later phases.

## Architecture boundary

Gameplay/runtime must depend on a persistence contract, not SQL calls scattered through world code.

```text
WorldRuntime / Session
        ↓
PersistenceService
        ↓
IPersistenceStore
        ↓
SQLiteStore (F06 first adapter)
        ↓
Database/Migrations
```

Recommended code ownership after F06 begins:

```text
Server/
└── Persistence/
    ├── PersistenceStore.hpp
    ├── PersistenceService.hpp/.cpp
    └── SQLiteStore.hpp/.cpp

Database/
├── Migrations/
└── Seeds/
```

`WorldRuntime` must not include `sqlite3.h` and must not know SQL table names.

## Initial database engine

**Prepared recommendation: SQLite for F06's first adapter.**

Reason:

- zero external service is required for local Windows development;
- deterministic CI and integration tests are much easier;
- the first Fantasy Server is a single authoritative process;
- database files can be temporary per test;
- the persistence API can be proven before introducing production infrastructure.

This is not a claim that SQLite must remain the final public-server database. Before website/public-account integration or production scaling, a PostgreSQL-class adapter can be added behind the same persistence boundary if the workload requires it.

Database files remain runtime artifacts and must never be committed.

## Minimal schema model

The first schema should stay intentionally small.

### `schema_migrations`

Tracks applied migrations.

Minimum fields:

```text
version
name
applied_at
```

### `accounts`

Local/runtime identity only in F06.

Minimum fields:

```text
id
handle
created_at
updated_at
```

Rules:

- `handle` unique after normalization;
- no plaintext password column;
- F06 development login may resolve a local account without defining final public authentication.

### `characters`

Minimum fields:

```text
id
account_id
name
position_x
position_y
position_z
revision
created_at
updated_at
```

Rules:

- `name` unique according to one documented normalization rule;
- position is Server-authoritative;
- `account_id` references an existing account;
- `revision` supports stale-write detection/optimistic save checks;
- character position must reference a valid walkable FMAP tile when loaded into WorldRuntime.

## Save/load rules

1. The Client never writes database state directly.
2. Position saved is the Server's current authoritative entity position.
3. A load returning an invalid/non-walkable position must not silently place the character inside invalid terrain.
4. Migration failure must stop startup instead of running with a partially migrated schema.
5. Character save must be transactional.
6. Clean disconnect should request save before entity removal is considered complete.
7. Abrupt disconnect handling may save the last authoritative state through a controlled cleanup path.
8. Database failure must return an explicit Server error; it must not be converted into a fake successful save.

## Development login transition

`LoginDev` remains local-only during F06.

A safe evolution is:

```text
LoginDev(characterName)
        ↓
local development account resolution
        ↓
character lookup/create through PersistenceService
        ↓
WorldRuntime entity spawn at saved position
```

Do not turn `LoginDev` into production authentication. Public authentication is a separate security phase.

## Migration discipline

Migrations are append-only once merged and used by evidence.

Suggested naming:

```text
Database/Migrations/
0001_initial_persistence.sql
0002_<next_change>.sql
```

Requirements:

- migrations run in numeric order;
- an already-applied version is never re-run;
- checksum/name mismatch must be treated as an error rather than silently ignored;
- tests create a fresh temporary DB from migration zero;
- tests also reopen an existing migrated DB.

## F06 test gates

F06 cannot be PASS without all of these:

```text
PERSIST-01  fresh database migrates successfully
PERSIST-02  second startup detects schema already current
PERSIST-03  account create/load roundtrip
PERSIST-04  character create/load roundtrip
PERSIST-05  saved coordinates survive process/runtime restart
PERSIST-06  invalid saved position is rejected or recovered by explicit documented policy
PERSIST-07  duplicate normalized account/character names are rejected
PERSIST-08  transaction rollback leaves previous state intact on forced failure
PERSIST-09  reconnect loads the latest authoritative saved position
PERSIST-10  no database/build/log artifacts are tracked by Git
```

The decisive end-to-end gate is:

```text
Client login
 → persisted character loaded
 → Server enters FMAP
 → move
 → save
 → Server stop
 → Server restart
 → same character returns at saved tile
```

## Failure policy

For persistence failures:

```text
STOP
 ↓
identify layer (migration / adapter / service / session / runtime)
 ↓
fix smallest layer
 ↓
unit test that layer
 ↓
rerun persistence integration test
 ↓
rerun F05 native-play regressions
```

Do not solve database bugs by adding SQL directly to `WorldRuntime` or `DevelopmentSession`.

## Activation rule

F06 implementation begins only after:

- F03 interactive Windows gate = PASS;
- F05 interactive Windows first-play/reconnect = PASS;
- `docs/evidence/F05/WINDOWS-VALIDATION.md` is complete;
- final regression is green;
- F03/F05 closeout commit is pushed and its GitHub Actions run succeeds.

Until those conditions are met, this file is planning only.
