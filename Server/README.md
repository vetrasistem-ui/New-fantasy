# Server

Runtime nativo do Fantasy.

```text
Server/
├── Core/
├── Modules/
├── Config/
└── Logs/
```

O objetivo é construir **Fantasy Server**, não incorporar TFS como core permanente.

## Estado atual — F04

O servidor já deixou de ser apenas um skeleton de toolchain. O primeiro slice do World Runtime está ativo:

```text
FMAP
 ↓
WorldRuntime
 ↓
Global Tile Index
 ↓
Entities / Movement / Walkability
 ↓
Tick Scheduler
```

O smoke test carrega `Game/Maps/World/world.fmap.json` diretamente, inicializa o runtime, cria uma entidade de desenvolvimento, move essa entidade e encerra o processo de forma limpa.

Também existem testes headless para movimento atravessando chunks, ocupação de tile, scheduler e ciclo Start/Stop.

## Build / tests

```powershell
cmake -S Server -B build/server
cmake --build build/server --config Release
ctest --test-dir build/server -C Release --output-on-failure
```

## Direção

O core deve crescer em módulos pequenos:

```text
App
World
Map
Entities
Scheduler
Network
Protocol
Persistence
```

Antes de F04 PASS, o modelo/IO FMAP reutilizável será extraído para `Shared/`, eliminando a dependência temporária do Server sobre a implementação localizada em `Studio/MapEngine`.

TFS 1.4.2 permanece somente como referência de comportamento em `.upstream/`.
