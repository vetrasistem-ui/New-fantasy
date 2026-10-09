# Fantasy Module Manager V1

## Decision

Fantasy will use the simplest functional module architecture for reusable game systems.

V1 is **not** an advanced patch engine. A system is installed as an isolated module inside the Fantasy project and is composed into the target runtime during Validate / Play Test / Build.

Primary goals:

- simple to understand;
- safe to install/remove;
- reusable across projects;
- compatible with System Lab;
- compatible with TFS1098 + OTC Extended;
- no direct binary patching;
- no intelligent source-code merge in V1.

## Project layout

```text
<Project>/
  Game/
  Assets/
  Systems/
    quest/
      system.json
      config.json
      logic/
      server/
      client/
      assets/
      database/
    capture/
      system.json
      config.json
      logic/
      server/
      client/
      assets/
      database/
```

Each module owns its own files. A module must not silently edit another module's source files.

## Minimal manifest

Example `system.json`:

```json
{
  "schemaVersion": 1,
  "id": "capture",
  "name": "Capture System",
  "version": "1.0.0",
  "enabled": true,
  "requires": ["inventory", "creatures"],
  "compatibility": ["tfs1098", "otc_extended"]
}
```

V1 manifest fields:

- id;
- name;
- version;
- enabled;
- dependencies;
- compatible runtime/profile;
- optional user-facing configuration schema;
- optional database migration list;
- optional server/client/assets contributions.

## Library versus installed modules

Fantasy Studio has two concepts:

```text
Systems Library
  -> reusable packages stored by the Studio

Project/Systems
  -> modules installed into the current game
```

The Library may contain:

- official Fantasy modules;
- user-created modules;
- imported modules;
- templates.

## V1 user flow

### Create / package

```text
System Lab
  -> Create or edit system
  -> Validate
  -> Test
  -> Export package
  -> Systems Library
```

The first package format may be a normal ZIP internally. A future `.fsystem` extension can wrap the same structure.

### Install

```text
Systems Library
  -> Select module
  -> Check dependencies
  -> Copy module into Project/Systems/<id>/
  -> Register installed version
  -> Project Health
  -> Ready for Play Test / Build
```

The installer must not patch `tfs.exe`, Fantasy executables or arbitrary project source files.

### Enable / disable

A module can be disabled without deleting it.

Disabled modules remain in the project but are omitted from runtime composition.

### Remove

```text
Installed Systems
  -> Select module
  -> Verify no enabled module depends on it
  -> Disable
  -> Remove project module directory if requested
  -> Keep persistent game data by default
  -> Project Health
```

V1 does not automatically drop database tables/data on uninstall.

## Runtime composition

The Fantasy project remains the source of truth.

Modules are translated/staged only during runtime preparation or build:

```text
Fantasy Project
  + enabled Systems
        |
        v
Project Health
        |
        v
Runtime composition
        |
        +-- TFS1098 adapter
        |     +-- Lua/events/config/database migrations
        |
        +-- OTC Extended adapter
              +-- modules/UI/semantic channels
```

An authored module works with semantic concepts and channels. It must not depend directly on fixed numeric extended opcodes.

## Configuration

A module should expose configurable parameters separately from its internal implementation.

Example:

```json
{
  "captureChance": 20,
  "bossCapture": false,
  "healthBonus": true
}
```

The Studio can render those settings without requiring the user to edit the module logic.

## Dependencies

V1 supports required module IDs and minimum compatible versions.

Example:

```text
capture
  requires creatures >= 1.0
  requires inventory >= 1.0
```

Installation fails cleanly when a required dependency is missing.

Removal is blocked when another enabled module depends on the selected module.

Circular dependencies are invalid and must be reported by Project Health.

## Database behavior

A module may include forward-only V1 migrations.

The Studio/runtime records applied module migration IDs and versions.

V1 rules:

- apply each migration once;
- never store credentials in a module;
- uninstall does not automatically destroy persistent data;
- destructive down migrations are deferred to a future version.

## Update behavior

V1 uses whole-module replacement, not intelligent merge.

```text
capture 1.0
  -> backup installed module folder
  -> stage capture 1.1
  -> validate dependencies and Project Health
  -> activate 1.1
```

If validation fails, restore the previous module folder.

If the user has manually modified the installed module, V1 should warn before replacement. Advanced three-way merge/conflict resolution is intentionally deferred.

## V1 Studio surface

```text
Systems
  Installed
  Library
  My Systems
```

Minimum actions:

```text
Import
Install
Enable
Disable
Configure
Open in System Lab
Test
Export
Update
Remove
```

## Integration with existing Fantasy components

Module Manager V1 should reuse:

- Fantasy System definitions;
- System Lab contracts;
- FantasyAuthoringRepository;
- dependency validation;
- semantic channels;
- Project Health;
- Build Pipeline;
- Runtime Profiles;
- TFS1098 / OTC Extended adapters.

It should not introduce a parallel persistence architecture.

## Explicitly deferred

The following are NOT part of V1:

- binary executable patching;
- arbitrary source-code patching;
- intelligent file merge;
- automatic conflict resolution;
- marketplace/payments;
- signed commercial packages;
- automatic destructive database rollback;
- automatic C++ backend patching;
- community dependency registry.

## Acceptance criteria

Module Manager V1 is functional when a user can:

1. create or import a module;
2. see it in the Studio Library;
3. install it into a Fantasy project;
4. detect missing dependencies before activation;
5. configure its exposed settings;
6. enable/disable it;
7. include/exclude it deterministically during runtime preparation/build;
8. update it using whole-module replacement with backup;
9. remove it without corrupting the project;
10. pass Project Health after each operation.

This simple module model is the official first implementation. Advanced patching can be added later only when a real use case requires modifying content outside the module boundary.
