# Fantasy Knowledge Compiler V1

Permanent, independent Python subsystem for factual map evidence and derived
structural knowledge. Uses only the Python standard library; no CMake, pip,
GUI, asset renderer, or PDB dependency. The existing C++ Map Atlas is preserved
as a future differential-validation implementation.

Requires Python 3.10+ with SQLite window-function support. Acceptance was run
with the already available Codex Python runtime, without installing packages.

## Data boundary

`FantasyEvidence.sqlite` (`fantasy-evidence-v1`) stores actual source positions,
raw item order, nested containers, properties, towns, houses, waypoints and
spawns. Source paths are relative to the repository root; SHA256 and byte sizes
identify the exact inputs. Unknown nodes/attributes have byte-offset diagnostics
and retained raw bytes. Item strings use reversible Latin-1 decoding.

`FantasyKnowledge.sqlite` (`fantasy-knowledge-v1`) stores aggregated statistics
and query provenance tied to the exact Evidence SHA256. It contains no copied
regions or coordinates. Up to three raw occurrence pointers per asset provide
representative evidence; their positions remain in Evidence. Planner integrations
should consume Knowledge, not Evidence.

Spatial examples are also limited to three center-tile pointers for each of the
25 most frequent corner and interruption keys. Each satisfies its miner predicate.

The physical OTBM reader and attribute widths were implemented independently
from Fantasy's own `Shared/Formats/Legacy/OtbmStreamReader.cpp`, `OtbmReader.cpp`
and `LegacyAuxXmlReader.cpp`. Ground behavior follows the boundary in
`Studio/MapEngine/Import/LegacyCanonicalMapBuilder.cpp`: the first registry-
classified Ground becomes the canonical ground. V1 has no registry input, so it
preserves **all** compact and child items in raw order and marks
`ground_resolution=unresolved`. Compact encoding alone is not a Ground classifier.
Ground transitions remain empty with an explicit unresolved diagnostic in metadata.

## Commands

```powershell
python Tools/Knowledge/fantasy_knowledge.py import `
  --otbm path/to/map.otbm --house path/to/map-house.xml `
  --spawn path/to/map-spawn.xml --evidence build/knowledge/FantasyEvidence.sqlite

python Tools/Knowledge/fantasy_knowledge.py compile `
  --evidence build/knowledge/FantasyEvidence.sqlite `
  --knowledge build/knowledge/FantasyKnowledge.sqlite

python Tools/Knowledge/fantasy_knowledge.py inspect `
  --knowledge build/knowledge/FantasyKnowledge.sqlite --asset 22280

python -m unittest discover -s Tools/Knowledge/fantasy_knowledge/tests -v
```

`import` and `compile` read inputs without writing them. Fingerprints are checked
before and after. Output uses a sibling `.building` database; only after integrity,
foreign-key and fingerprint checks does an atomic replace publish the final file.
A failure preserves the previous published database and keeps staging for diagnosis.
An existing staging file causes an explicit error rather than automatic deletion.
Outputs cannot be the inputs. Do not use source-pack paths as outputs.

Reader memory is limited to a 1 MiB physical buffer, current tile/container and
batches (default 20,000, configurable 100–50,000). Properties exceeding 16 MiB or
nesting beyond 128 fail explicitly. SQLite aggregates use disk-backed temporary
storage and a 64 MiB page cache; no complete map is assembled in Python RAM.
For constrained environments, set `SQLITE_TMPDIR` to a writable build directory.

## Statistical definitions and provenance recipes

All recipes `fkc-v1:<table>` use the fingerprint and parameters recorded in
`knowledge_provenance`. Confidence 1.0 describes the exact observed count,
**not** certainty about any semantic role.

| Recipe | Evidence predicate/count |
| --- | --- |
| asset_usage | All raw occurrences, including nested contents; distinct tile count; house membership from owning tile; floor min/max |
| asset_floor_usage | All raw occurrences grouped by server ID and owning tile floor |
| house_affinity | Inside/outside raw occurrence counts; ratio = inside / total |
| cooccurrence | Unordered distinct top-level ID pairs on one tile; once per tile; containers' contents excluded |
| adjacency | Distinct top-level presence across same-map/floor cardinal neighbors; E/S counted once physically, reversed pairs derive W/N |
| ground_transitions | Different resolved Ground IDs across E/S; reversed pairs derive W/N; unavailable without a registry |
| line_patterns | Maximal contiguous same-ID runs of length >=2; partition by map/floor/row or column; summarize run count, max and mean |
| corner_patterns | Same ID at center and two perpendicular cardinal neighbors; NE/NW/SE/SW; extra arms are allowed |
| interruption_patterns | Identity-family A-X-A across immediate cardinal neighbors; A absent from center and X != A |
| vertical_relationships | Distinct top-level IDs at same x/y, consecutive floors; +1 physically counted, -1 derived |

`corner_candidate` family rows have deliberately bounded heuristic confidence
(<=0.5), evidence counts and `candidate` status. No automatic wall, door, roof or
window names are assigned. Interruption family columns use individual IDs in V1.
`IdentityFamilyResolver` is the extension boundary for later asset families;
custom families require a unique family-presence projection rather than silently
reusing ID-specific statistics. `RegionMiner` reserves an eight-neighbor interface
and intentionally raises `NotImplementedError` until region algorithms are approved.

## Acceptance gates

1. Unit tests (including malformed nodes and failed atomic publication).
2. Existing small map import and compilation; positive tile/item/asset counts.
3. Global source import, cross-check actual counts, compile, verify integrity and
   input byte identity. Counts are observed, never coerced to expected values.

Artifacts and logs belong under ignored `build/knowledge`, never in Git. Successful
statistical compilation enables review and subsequent asset-family work; it does
not homologate a building grammar, brush engine, map writer or TFS runtime.
