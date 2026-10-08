# Vanilla TFS 1.4.2 OTBM Runtime Homologation

**Status:** PASS for generated OTBM load/runtime startup.

## Target runtime

The validation used the official upstream release package:

- project: `otland/forgottenserver`;
- release: `v1.4.2`;
- Git SHA: `31d6e85d`;
- official artifact: `tfs-v1.4.2-ubuntu-gcc.tar.gz`;
- protocol line: 10.98.

The package was downloaded from the official GitHub release by the repository workflow `TFS 1.4.2 Homologation Tool`. The TFS executable was not rebuilt or patched.

## Fantasy output under test

The tested map was the new OTBM written by Fantasy from the pinned real 10.98 `global_dash` project:

```text
source canonical tiles: 6,106,271
writer items:           6,499,901
writer tile areas:      277
output bytes:            64,919,912
SHA-256: ae640e8b3a27b8ef108de56673c5b991ac99de5d6cfacb7516479ea2cbf74bed
```

The source OTBM was not overwritten.

Runtime item/map companions:

- pinned 10.98 `items.otb` used by the Fantasy import/export profile;
- `map-house.xml` from the pinned project;
- `map-spawn.xml` from the pinned project.

## Database harness boundary

The local execution environment did not contain a MariaDB/MySQL daemon. TFS performs a mandatory database connection before it reaches `IOMap`, so a minimal local MySQL-protocol startup harness was used only to satisfy the database bootstrap calls.

The harness returned startup/config responses and empty persistence result sets. It does **not** parse OTBM, OTB, house XML, spawn XML, items, towns, tiles or map nodes. Therefore it cannot make an incompatible map pass the TFS map loader.

This homologation proves the TFS file/runtime boundary, not production database behavior.

## Vanilla TFS result — PASS

Relevant upstream output:

```text
The Forgotten Server - Version v1.4.2
Git SHA1 31d6e85d dated 2022-05-08T20:27:14-04:00

>> Loading items
...
>> Loading map
> Map size: 30000x30000.
> Map loading time: 2.897 seconds.
...
>> Loaded all modules, server starting up...
>> Forgotten Server Online!
```

A repeat after placing the auxiliary XML files under the exact names preserved in the OTBM metadata produced:

```text
>> Loading map
> Map size: 30000x30000.
> Map loading time: 2.757 seconds.
```

There were no `IOMap`/OTBM format rejection messages and no house/spawn *file-not-found* errors after the filenames were corrected.

## Expected content warnings on vanilla datapack

The real project is a PokéTibia map, while the vanilla TFS datapack does not define its Pokémon/NPC content. Once the real `map-spawn.xml` was present, vanilla TFS correctly parsed the spawn file and emitted warnings such as:

```text
[Warning - Spawn::addMonster] Can not find Charizard
[Warning - Spawn::addMonster] Can not find Dragonite
[Warning - Spawn::addMonster] Can not find Lucario
```

It also reported missing project-specific NPC XML definitions. These are **datapack content gaps**, not OTBM writer failures. The important distinction is that TFS reached and processed the external spawn/NPC references after accepting the generated map.

## Gate conclusion

```text
Fantasy canonical MapDocument
        ↓
Fantasy OTBM v3 Writer
        ↓
Fantasy semantic reopen        PASS
        ↓
vanilla TFS 1.4.2 IOMap load   PASS
        ↓
TFS server startup / Online    PASS
        ↓
10.98 client login + walk      PENDING
```

The next mandatory compatibility gate is to connect a compatible 10.98 client to a TFS runtime backed by a real database/account/player fixture and verify login, character entry and movement on the exported map.
