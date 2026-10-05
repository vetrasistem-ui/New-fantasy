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

## F04 — World Runtime: PASS

O runtime carrega `Game/Maps/World/world.fmap.json` diretamente e possui:

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

Os testes cobrem movimento entre chunks, ocupação exclusiva, tile bloqueado, scheduler e ciclo Start/Ready/Tick/Stop. Studio e Server consomem a mesma implementação FMAP neutra em `Shared/Formats/FMAP/`.

## F05 — Protocol v1 + First Native Play

O Server já possui o caminho técnico do primeiro play nativo:

```text
TCP loopback
   ↓
FrameStream
   ↓
Fantasy Protocol v1
   ↓
DevelopmentSession
   ↓
WorldSnapshot
   ↓
WorldRuntime
   ↓
FMAP
```

A sessão implementa:

- `Hello / HelloAck` e version handshake;
- sequence monotônica do Client;
- `LoginDev / LoginOk`;
- `EnterWorld`;
- snapshot inicial `MapChunk` com FMCP v1;
- `EntityAdd`;
- `MoveRequest(direction)` validado pelo Server;
- `EntityMove` com posição autoritativa;
- `Disconnect` limpo;
- cleanup da entidade se a conexão TCP cair abruptamente.

No login, o Server envia os chunks da região/piso inicial. Grounds, objects e tags continuam semânticos; nenhum ID legado Tibia participa do caminho.

## Modo de validação atual

```powershell
build/server/Release/fantasy-server.exe --serve-once 17171
```

`--serve-once`:

- liga somente em `127.0.0.1`;
- aceita uma conexão;
- executa uma sessão F05;
- encerra após Disconnect limpo.

Ele existe para validação determinística. Ainda **não** é o servidor multi-client/24×7 final.

`LoginDev` também é exclusivamente de desenvolvimento e não pode ser exposto publicamente.

## Build / testes

```powershell
cmake -S Server -B build/server
cmake --build build/server --config Release
ctest --test-dir build/server -C Release --output-on-failure
```

O conjunto atual cobre runtime, codec do protocolo, FMCP, sessão autoritativa, TCP loopback real, first-play, reconnect e cleanup por desconexão abrupta.

Para validar Server e Client como processos separados:

```powershell
./scripts/run-native-play.ps1 -Configuration Release -Port 17171
```

## Limites deliberados antes de F06+

Ainda não entram no fechamento F05:

- contas persistentes;
- autenticação pública;
- banco de personagens;
- servidor multi-client 24/7;
- inventário/combate/creatures completos;
- assets finais;
- TLS/infra pública.

Esses sistemas não devem ser antecipados para contornar falhas do first-play nativo.

## Direção

O core cresce em módulos próprios e pequenos:

```text
App
World
Map
Entities
Scheduler
Network
Protocol
Persistence
Gameplay
```

TFS 1.4.2 permanece somente como referência comportamental isolada em `.upstream/`.
