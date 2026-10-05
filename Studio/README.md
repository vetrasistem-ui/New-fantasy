# Studio

Código do Fantasy Studio.

```text
Studio/
├── Core/
├── MapEngine/
├── Project/
├── Runtime/
├── Automation/
└── UI/
```

O Studio não nasce de uma cópia do RME. O `MapEngine/` é uma implementação própria baseada no FMAP e no núcleo compartilhado em `Shared/Formats/FMAP/`.

RME 3.7 permanece em `.upstream/` apenas como referência de comportamento, fixtures e eventual interoperabilidade legada.

## Estado atual

**F01 — Project System: PASS**

- `fantasy.project.json`;
- caminhos relativos;
- New/Open/Recent Project;
- Open Main Map;
- relocation validation.

**F02 — Fantasy Map Core: PASS**

- World / Region / Chunk / Tile;
- semantic asset keys;
- load/save FMAP;
- validation;
- transactions;
- rollback;
- undo/redo;
- roundtrip semântico.

**F03 — Fantasy Map Editor MVP: TECHNICAL PASS**

O editor nativo `fantasy-studio-gui.exe` usa SDL3 + SDL_GPU + Dear ImGui e já possui:

- viewport 2D;
- floors;
- pan/zoom;
- seleção de tile;
- ground brush;
- object add/remove;
- Fill conectado atravessando fronteiras de chunk;
- Erase;
- Undo/Redo;
- minimapa básico;
- Save FMAP;
- reabertura do mesmo mapa pelo Project Manager.

A alteração visual passa sempre pelo `MapDocument`/operações de domínio; a GUI não possui um segundo modelo de mapa.

## Gate ainda pendente

Falta somente o gate **interativo Windows real** da F03: abrir o editor, editar visualmente, usar Undo/Redo/Fill/Object/Erase, salvar, fechar, reabrir e confirmar persistência visual com screenshots.

Esse roteiro está integrado ao handoff:

```text
docs/CODEX-F05-WINDOWS.md
```

A fixture canônica `Game/Maps/World/world.fmap.json` deve ser restaurada após a captura de evidências.

## Princípios permanentes

- GUI e Codex usam o mesmo Map/Core API;
- FMAP é a fonte de verdade do mapa;
- operações grandes são reversíveis/transacionais;
- nenhuma ferramenta procura arquivos principais em locais arbitrários;
- `fantasy.project.json` define caminhos oficiais;
- OTBM/10.98 não entram no caminho nativo do Studio;
- mudanças de FMAP devem ser versionadas e testadas;
- automação futura deve operar o mesmo núcleo usado pela GUI.
