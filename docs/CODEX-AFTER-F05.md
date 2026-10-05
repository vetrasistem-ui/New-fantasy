# Codex — After F05 Closeout

Status: **DO NOT EXECUTE UNTIL F03 AND F05 ARE FORMALLY PASS**

This file defines the next ordered implementation contract so the project can continue immediately after the Windows visual closeout without re-planning F06 from scratch.

Primary design reference:

```text
docs/F06-PERSISTENCE-PLAN.md
```

## Entry condition

Do not start this sequence unless all are true:

```text
F03 = PASS
F05 = PASS
docs/evidence/F05/WINDOWS-VALIDATION.md complete
final F05 regression = PASS
GitHub Actions green on the exact closeout SHA
working tree clean
```

If any condition is false, return to `docs/CODEX-NEXT.md` / `docs/CODEX-F05-WINDOWS.md`.

---

# P00 — Freeze F06 baseline

Record:

```powershell
git checkout main
git pull --ff-only
git status --short
git rev-parse HEAD
```

Create/update F06 evidence with starting SHA:

```text
docs/evidence/F06/RESULT.md
```

Do not change protocol/FMCP/FM﻿AP semantics during P00.

---

# P01 — Persistence contract only

Implement the database-independent persistence API first.

Target boundary:

```text
Server/Persistence/
├── PersistenceStore.hpp
└── PersistenceService.hpp/.cpp
```

Minimum domain operations:

```text
ensureDevelopmentAccount(handle)
findAccount(...)
createCharacter(...)
findCharacter(...)
loadCharacter(...)
saveCharacterPosition(...)
```

Rules:

- no SQL in WorldRuntime;
- no SQL in Client;
- no DB engine types in protocol contracts;
- no public authentication yet;
- server remains authoritative.

Gate: compile + unit tests with an in-memory/fake store before adding SQLite.

---

# P02 — Migration runner

Add append-only migrations under:

```text
Database/Migrations/
```

Implement migration discovery/order/application and `schema_migrations` tracking.

Required tests:

```text
fresh DB → all migrations apply
second startup → no migration re-run
out-of-order/duplicate version → reject
migration failure → startup fails cleanly
```

Do not proceed if migration handling can leave a partial schema without a detectable failure.

---

# P03 — SQLite adapter

Implement:

```text
Server/Persistence/SQLiteStore.hpp/.cpp
```

It must satisfy the same persistence contract proven in P01.

Minimum schema:

```text
schema_migrations
accounts
characters
```

No inventory/skills/quests in F06.

Gate: fresh temporary DB + reopen existing DB + transaction rollback tests.

---

# P04 — Character persistence service

Connect account/character domain logic to the store.

Required behavior:

```text
local development account
   ↓
character create/load
   ↓
validated saved position
```

Add explicit handling for:

- duplicate normalized names;
- missing character;
- invalid saved position;
- stale revision/save conflict;
- database error.

Do not hide persistence errors by spawning a fake successful character.

---

# P05 — Session integration

Evolve the local-only F05 path carefully:

```text
LoginDev(characterName)
   ↓
PersistenceService
   ↓
load/create persisted character
   ↓
WorldRuntime add entity at saved position
```

On movement, WorldRuntime remains authoritative.

On clean Disconnect:

```text
current authoritative position
   ↓
save transaction
   ↓
entity cleanup
```

Abrupt disconnect must use one documented cleanup/save policy.

Do not convert `LoginDev` into public authentication.

---

# P06 — Restart persistence integration test

Create an automated integration gate that proves:

```text
start Server/runtime
login character
initial position loaded
move to new valid tile
save/disconnect
stop
recreate Server/runtime + persistence store
login same character
position == previously saved tile
```

This must use a temporary database and must not rely on a developer's machine state.

Also prove reconnect without restart.

---

# P07 — Failure and integrity tests

Required negative gates:

```text
invalid stored position
forced transaction failure
stale revision
missing account relation
normalized duplicate account handle
normalized duplicate character name
corrupt/unavailable DB open
```

Expected behavior must be explicit and deterministic.

No silent fallback to spawn unless the policy is documented and tested.

---

# P08 — Full regression and F06 closeout

Run:

```powershell
./scripts/build-native.ps1 -Configuration Release
./scripts/run-native-play.ps1 -Configuration Release -Port 17201 -Character "F06 Regression Hero"
git status --short
git diff --check
```

Then run the new persistence/restart integration gate.

Required before F06 PASS:

- F00–F05 regressions remain green;
- persistence tests green;
- database files are ignored/not staged;
- no absolute paths persisted;
- no OTBM/10.98/TFS schema introduced;
- no public listener/auth added;
- Windows CI green on the exact final SHA.

Update only after evidence:

```text
docs/evidence/F06/RESULT.md
docs/ROADMAP.md
AGENTS.md
README.md
```

Then advance to F07 — Item / Inventory Core.

---

# Mandatory implementation discipline

For every P-stage:

```text
implement smallest slice
 ↓
compile
 ↓
direct tests
 ↓
relevant integration test
 ↓
commit with one clear scope
 ↓
continue
```

Never combine database foundation, inventory, auth, website and public multiplayer in one phase.
