# Client

Cliente oficial do Fantasy.

```text
Client/
├── Core/
├── Modules/
├── UI/
└── Assets/
```

O alvo final é um **Fantasy Client** falando `Fantasy Protocol`, com UI, assets e sistemas próprios. OTClient 10.98 permanece apenas como referência isolada em `.upstream/`.

## Estado atual — F05

O primeiro Client nativo já possui um core independente da UI:

```text
DevelopmentClient
   ↓
Shared/Network
   ↓
Fantasy Protocol v1
   ↓
MapChunk + FMCP v1
```

O core executa:

- handshake `Hello / HelloAck`;
- `LoginDev` local;
- `LoginOk / EnterWorld`;
- recepção e reconstrução de chunks FMAP sem OTBM;
- resolução global `regionOrigin + chunkOffset + tileLocal`;
- `EntityAdd`;
- envio de `MoveRequest(direction)`;
- aplicação de `EntityMove` autoritativo do Server;
- Disconnect limpo.

Existem dois executáveis de desenvolvimento:

```text
fantasy-client.exe      # headless / testes / automação
fantasy-client-gui.exe  # primeiro visual nativo F05
```

O cliente visual usa SDL3 + SDL_Renderer3 + Dear ImGui apenas como camada de apresentação F05. Ele desenha os tiles semânticos recebidos com cores determinísticas provisórias e o player na posição enviada pelo servidor. As setas enviam intenções de movimento; o Client nunca define sua posição de forma autoritativa.

`SDL_Renderer3` é o baseline 2D oficial. `SDL_GPU` fica reservado para um backend avançado futuro, sem alterar `DevelopmentClient`, protocolo, FMCP ou autoridade do Server.

## Build / testes

```powershell
cmake -S Client -B build/client
cmake --build build/client --config Release
ctest --test-dir build/client -C Release --output-on-failure
```

Para o primeiro play em dois processos, após Server e Client estarem compilados:

```powershell
./scripts/run-native-play.ps1 -Configuration Release
```

O listener F05 usa `LoginDev` e é **somente local/loopback**. Não deve ser exposto publicamente.

## Próximo gate visual

Uma sessão Windows real ainda deve validar `fantasy-client-gui.exe` conectado ao `fantasy-server.exe`, confirmar os quatro chunks/64 tiles, mover com as setas, fechar, reconectar e registrar evidência visual. Assets/sprites finais vêm depois desse gate.
