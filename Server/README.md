# Server

Runtime nativo do Fantasy.

```text
Server/
├── Core/
├── Network/
├── Modules/
├── Config/
└── Logs/
```

O objetivo é construir **Fantasy Server**, não incorporar TFS como core permanente.

## F04 — PASS

O runtime nativo carrega `Game/Maps/World/world.fmap.json` diretamente e possui:

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

Os testes headless cobrem movimento entre chunks, ocupação de tile, tile bloqueado, scheduler e ciclo Start/Stop. Studio e Server consomem a mesma implementação FMAP neutra em `Shared/Formats/FMAP/`.

## F05 — primeiro play nativo

O Server agora possui o caminho de desenvolvimento:

```text
TCP loopback
   ↓
FrameStream / Fantasy Protocol v1
   ↓
DevelopmentSession
   ↓
WorldSnapshot
   ↓
WorldRuntime
   ↓
FMAP
```

No login, o Server envia `LoginOk`, `EnterWorld`, os `MapChunk` do piso/região inicial e `EntityAdd`. O conteúdo dos chunks usa FMCP v1, mantendo grounds/objects semânticos. Movimento é `MoveRequest(direction)` e a resposta é `EntityMove` com posição autoritativa.

O executável suporta um modo determinístico de validação em um único cliente:

```powershell
build/server/Release/fantasy-server.exe --serve-once 7171
```

Esse modo liga **somente em `127.0.0.1`**, aceita uma sessão, aguarda Disconnect limpo e encerra. `LoginDev` é propositalmente uma autenticação de desenvolvimento e não pode ser usado como endpoint público.

## Build / testes

```powershell
cmake -S Server -B build/server
cmake --build build/server --config Release
ctest --test-dir build/server -C Release --output-on-failure
```

O `fantasy-native-play-tests` usa TCP loopback real e exercita Client → Server → FMAP, incluindo reconnect.

Para validar os executáveis Server e Client como dois processos separados:

```powershell
./scripts/run-native-play.ps1 -Configuration Release
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
