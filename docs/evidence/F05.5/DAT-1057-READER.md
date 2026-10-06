# F05.5 — DAT 10.57 reader validation

Status: **IMPLEMENTED / LOCAL REAL-PACK PARSE PASS / CI PENDING**

## Scope

This checkpoint adds an isolated DAT 10.57 metadata reader for the first compatibility profile, PokeFans 10.98. It remains inside `Shared/Assets/Legacy/` and does not introduce DAT concepts into FMAP, `MapDocument`, Server world state or the Fantasy Protocol.

The reader covers the item + creature appearance section required to resolve OTB client IDs into sprite references. Effects and distance effects are deliberately not interpreted in this checkpoint; their declared counts are preserved in `DatHeader` for later support.

## Reference behavior

The implementation follows the 10.57-era layout used by the homologated Remere reference:

- 32-bit DAT signature followed by item max id, creature count, effect count and distance count;
- 10.10+ metadata flag shift, including No Movement Animation at encoded flag 16;
- 10.57 creature frame groups;
- 10.50+ animation timing records;
- 32-bit sprite IDs for the extended format.

Unknown flag layouts fail closed instead of advancing with guessed byte sizes.

## Synthetic regression

`fantasy-legacy-binary-readers-tests` now creates a deterministic DAT 10.57 fixture and validates:

- header decoding;
- shifted metadata flags and payload fields;
- item frame dimensions;
- animation loop/start/durations;
- 32-bit sprite IDs;
- creature frame-group decoding;
- item lookup bounds.

Existing SPR and OTB reader checks remain in the same test executable.

## Real PokeFans pack local validation

The owner-provided PokeFans 10.98 `Tibia.dat` was parsed locally without modifying the source file.

Observed header:

```text
signature:       0x42A3
item max id:     37228
creature count:  4206
effect count:    1093
distance count:  197
```

Parsed item + creature appearances:

```text
37129 items + 4206 creatures = 41335 appearances
```

A spot check of item 100 returned one frame group with sixteen 32-bit sprite references; the first sprite reference was 136.

The real pack's effects/distance-effect tail is intentionally left unread by this milestone, so this result must not be described as full DAT coverage yet.

## Next gate

Wire the matching `items.otb` serverId -> clientId mapping to DAT appearance metadata and then to SPR decoding through the shared `FantasyAssetRegistry`. The resulting loader must report missing OTB mappings, missing DAT appearances and missing SPR records instead of silently dropping them.
