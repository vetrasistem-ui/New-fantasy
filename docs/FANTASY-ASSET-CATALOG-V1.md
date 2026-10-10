# Fantasy Asset Catalog V1

## Purpose

Fantasy Asset Catalog V1 is the semantic layer between technical asset identifiers and higher-level authoring tools.

The catalog lets Fantasy tools reason about concepts such as `terrain.sand`, `terrain.water`, `structure.wall`, `structure.roof`, and `nature.tree` without teaching AI, procedural tools, or the map editor to guess directly from raw DAT/SPR/OTB ids.

The existing legacy asset registry remains the technical mapping source for 10.98 assets. The catalog adds meaning on top of that registry; it does not replace or modify DAT, SPR, OTB, or OTBM.

```text
DAT / SPR / OTB
      |
      v
Legacy Asset Registry
      |
      v
Fantasy Asset Catalog
(tags + families + roles)
      |
      +----> Brushes
      +----> Map commands
      +----> future AI world/map authoring
      |
      v
Fantasy Map Core -> OTBM
```

## V1 scope

V1 is deliberately small and functional.

It supports:

- one project catalog at `Assets/Catalog/asset-catalog.json`;
- a catalog bound to an existing Fantasy asset profile;
- semantic entries;
- tags;
- asset families;
- roles inside a family;
- weighted variants;
- optional family-to-brush reference;
- lookup by semantic id;
- lookup by required tags;
- lookup by family and role;
- deterministic variant selection from a seed;
- resolution from a semantic entry to a legacy Registry/server id;
- structural validation and persistence;
- Project Health validation when a catalog exists.

V1 does **not** attempt to classify all sprites automatically and does not use computer vision to guess asset meaning.

## Why the catalog exists

A generator must not need to write this:

```text
place item 4526
place item 4527
```

It should be able to reason in this vocabulary:

```text
terrain.sand
terrain.water
structure.wall.stone
structure.roof.red
nature.tree
```

The catalog resolves that semantic vocabulary to the concrete asset references used by the selected asset profile.

## Families and roles

A visual concept usually consists of more than one asset. Sand can have center variants, edges, corners, details, and transitions. A stone wall can have straight pieces, corners, doors, and decorations.

V1 groups these assets into families.

Example:

```text
Family: terrain.sand.basic
Tags: terrain, sand, ground
Brush: brush.terrain.sand

Members:
  terrain.sand.center.a   role=center weight=1
  terrain.sand.center.b   role=center weight=3
  terrain.sand.edge.north role=north_edge
```

The AI or procedural generator asks for a family/role. Fantasy chooses the concrete variant deterministically and the Brush layer remains responsible for border/transition behavior.

## JSON example

```json
{
  "schemaVersion": 1,
  "profileId": "assets.1098.official",
  "families": [
    {
      "id": "terrain.sand.basic",
      "name": "Basic Sand",
      "tags": ["terrain", "sand", "ground"],
      "brushRef": "brush.terrain.sand"
    }
  ],
  "entries": [
    {
      "id": "terrain.sand.center.a",
      "source": "legacy_registry",
      "assetRef": "legacy.pokefans1098.item.4526",
      "familyId": "terrain.sand.basic",
      "role": "center",
      "tags": ["terrain", "sand", "ground", "desert", "beach"],
      "weight": 1
    }
  ]
}
```

The ids in this example only illustrate the contract. Real classifications must be confirmed against the actual project asset profile; Fantasy must not guess semantic meaning from an id alone.

## Source kinds

V1 accepts two source kinds:

- `legacy_registry`: references an entry from `FantasyAssetRegistry` (DAT/SPR/OTB-backed profiles such as the current 10.98 path);
- `modern_asset`: references a Fantasy `ModernAssetDefinition`.

This allows the same semantic vocabulary to survive a future transition from legacy assets to modern/original Fantasy assets.

## Project Health

The Asset Catalog is optional in V1 so existing projects remain valid.

When a catalog exists, Project Health validates:

- catalog schema and structural rules;
- the referenced asset profile exists;
- optional family brush references exist.

Legacy registry reference validation is also available from `FantasyAssetCatalog::validateLegacyReferences(...)` for code paths that have the loaded runtime registry.

## AI contract

Future AI/map tooling should consume semantic queries instead of raw sprite ids.

Examples:

```text
find tags: terrain + sand
family: terrain.sand.basic, role: center
family: structure.wall.stone, role: segment
semantic id: terrain.water.center
```

For a legacy asset, Fantasy can then resolve:

```text
semantic id
  -> catalog entry
  -> legacy semantic registry key
  -> server/client/sprite mapping
```

This keeps the AI independent of Tibia-specific numeric ids while preserving full compatibility with the current 10.98 runtime.

## Next step after V1 validation

Do not build a large AI world generator yet.

After V1 passes CI and real asset use, the next practical step is to populate a small confirmed vocabulary needed for map construction, for example:

- grass;
- sand;
- water;
- dirt/road;
- rock/mountain;
- wall;
- roof;
- door/window;
- tree/bush/flower;
- bridge/fence;
- common indoor furniture.

That confirmed vocabulary can then be consumed by a small set of map commands and, later, higher-level region/city/route generators.
