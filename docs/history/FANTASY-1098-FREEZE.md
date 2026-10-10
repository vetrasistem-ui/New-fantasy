# Fantasy 10.98 / FKC V1 — historical freeze

Date: 2026-10-10
Decision: owner-authorized F00-CLOSE, pre-modernization baseline preparation.

## Active direction

10.98 is no longer the active development target. PokeFans is no longer the
primary fixture. TFS 1.4.2 and OTCv8 leave the active path and remain historical
compatibility references. This explicit owner decision freezes the previous
10.98 operational direction; it does not establish or implement a replacement
runtime, client or asset format.

Standard Base 15.25 is the next definition phase. No Canary, client 15.25,
appearances implementation, asset conversion or runtime migration starts here.
Existing documents describing the old baseline remain historical evidence;
their formal consolidation belongs to the next definition phase.

## Completed FKC V1 gate

FKC V1 was validated successfully: 19 unit tests, a small synthetic-map gate,
and global PokeFans Evidence/Knowledge acceptance. Historical evidence remains
in `build/knowledge/pokefans1098/acceptance-report.json` and the accompanying
console logs. The global gate imported 6,106,271 tiles and 6,499,901 items,
verified SQLite integrity and source fingerprints. This close operation reruns
the quick unit suite and records the result in
`build/audit/pre-modernization-close-report.json`; it does not rerun the global
map import.

Preserve the independent evidence, provenance and mining architecture. Supported
OTBM/sidecar contracts still require validation against any future input format.
Region mining remains an intentional V1 interface stub; statistical evidence is
not a validated semantic wall/door/roof classification.

## Atlas 10.98 status: FROZEN_INCOMPLETE

The C++ Atlas/CMake gates remain incomplete. Atlas did not pass its pending C++
build and small-map acceptance gates. FKC acceptance does not substitute for
those gates. Under this freeze decision, these pending historical gates are
not blockers for defining the modernization direction.

## Preservation and controlled housekeeping

No old data is discarded. Before removal of the 240 individually classified
untracked `PRESERVE_LOCAL_ONLY` files from the working tree, external copies
are created with their original relative paths and validated by SHA256 under
`C:/fantasy-preservation/pre-modernization-1098/`. The external `manifest.json`
records original/copy locations, sizes, hashes, classifications and subsystems.
The close report records the actual housekeeping outcome.

Ignored historical builds, previews, maps and FKC SQLite databases remain in
their existing local locations. The external manifest records the 36 FKC
artifact paths and hashes; it is not an external backup of those databases.
Six distribution archives remain local and receive exact-path ignore rules.
No broad third-party source-directory ignore rule is introduced.

The 26 FKC and 12 other native version candidates remain unchanged for a
reviewed historical baseline. Legacy-only local files are externally preserved.
All 42 mixed files are marked `MIXED_NEEDS_MODERNIZATION` in the close report;
none is converted or refactored in this phase.

No commit, push or merge is part of F00-CLOSE.
