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

## Estado atual

**F04 — Fantasy Server World Runtime: PASS.**

O caminho nativo atual é:

```text
Shared FMAP Core
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

Os testes headless cobrem movimento entre chunks, ocupação de tile, tile bloqueado, scheduler e ciclo Start/Stop.

O modelo/IO/validação FMAP reutilizável já foi extraído para `Shared/Formats/FMAP/`; Studio e Server consomem a mesma implementação neutra.

## F05 em andamento

A camada `Shared/Protocol/` está sendo transformada do contrato YAML em um codec binário real do Fantasy Protocol v1.

Primeiro fluxo-alvo:

```text
Client: Hello
Server: HelloAck
Client: LoginDev
Server: LoginOk + EnterWorld + MapChunk
Client: MoveRequest(direction)
Server: EntityMove(authoritative position)
```

O cliente envia intenção; posição e estado do mundo continuam sob autoridade do Server.

## Build / tests

```powershell
cmake -S Server -B build/server
cmake --build build/server --config Release
ctest --test-dir build/server -C Release --output-on-failure
```

## Direção

O core cresce em módulos pequenos:

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

TFS 1.4.2 permanece somente como referência de comportamento em `.upstream/`.
