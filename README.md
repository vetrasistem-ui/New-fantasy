# New Fantasy

New Fantasy é o novo ciclo da plataforma Fantasy: Studio, mapa, servidor, protocolo e cliente evoluindo para uma tecnologia própria, preparada desde o início para automação por IA.

## Direção oficial

O destino final **não é** um Remere's modificado, um TFS modificado ou um cliente preso ao protocolo 10.98.

A arquitetura alvo é:

```text
Fantasy Studio
      │
     FMAP
      │
Fantasy Server
      │
Fantasy Protocol
      │
Fantasy Client
```

TFS 1.4.2, RME 3.7 e um OTClient compatível com 10.98 permanecem fixados somente como **referências técnicas, oráculos de comportamento e fallback de desenvolvimento** durante a transição. Eles não definem o formato final da plataforma.

## Princípios

- Estrutura de pastas simples, previsível e sem duplicações.
- Caminhos persistidos sempre relativos à raiz do projeto.
- `FMAP` é a fonte de mapa nativa; OTBM é apenas referência/importação legada.
- `Fantasy Protocol` é o protocolo nativo; 10.98 é apenas referência/bridge temporária.
- `Fantasy Server` é próprio e autoritativo.
- Studio, CLI, scripts e Codex devem convergir nas mesmas operações de domínio.
- Operações de IA devem ser validáveis, determinísticas e reversíveis.
- Código externo não entra silenciosamente no produto; referências ficam isoladas em `.upstream/`.
- O jogo e o Studio crescem juntos depois do primeiro ciclo jogável nativo.
- O baseline visual 2D prioriza simplicidade e compatibilidade: SDL3 + SDL_Renderer3 + Dear ImGui; SDL_GPU é opcional para recursos avançados futuros.

## Estrutura

```text
New-fantasy/
├── Studio/
├── Game/
│   ├── Maps/
│   ├── Content/
│   ├── Assets/
│   ├── Scripts/
│   └── Config/
├── Server/
├── Client/
├── Shared/
│   ├── Protocol/
│   ├── Network/
│   └── Formats/
├── Database/
├── Tools/
├── Projects/
├── Tests/
├── scripts/
└── docs/
```

## Contratos próprios

- **FMAP v0**: fonte de mapa semântica, versionável e organizada por regiões/chunks.
- **FMCP v1**: payload binário semântico usado por `MapChunk` no Fantasy Protocol.
- **Fantasy Protocol v1**: framing TCP próprio, versionado, com servidor autoritativo.
- **Fantasy Project v2**: manifesto e resolução de caminhos do projeto.
- **Fantasy Data Model**: contratos compartilhados em evolução para entidades, itens, criaturas e conteúdo.

## Núcleo já funcional

### Fantasy Studio

- Project Manager nativo em C++20;
- New/Open/Recent Project;
- Open Main Map;
- caminhos relativos e relocation test;
- FMAP `World / Region / Chunk / Tile`;
- leitura/escrita/validação;
- transactions, rollback, undo e redo;
- editor visual SDL3 + SDL_Renderer3 + Dear ImGui;
- viewport 2D, floors, pan/zoom, brushes, Fill, Erase, minimapa e Save/Reopen.

### Fantasy Server

- carregamento direto de `Game/Maps/World/world.fmap.json`;
- índice global de tiles;
- entidades e ocupação;
- walkability e movimento autoritativo;
- travessia entre chunks;
- scheduler e ciclo Start/Ready/Tick/Stop;
- sessão de desenvolvimento independente de socket;
- transporte TCP loopback para o primeiro play nativo.

### Fantasy Protocol / Client

- envelope binário `FNTY` de 16 bytes;
- versão, message type, payload length e sequence;
- `Hello/HelloAck`;
- `LoginDev/LoginOk`;
- `EnterWorld`;
- `MapChunk` + FMCP v1;
- `EntityAdd/EntityMove/EntityRemove`;
- `MoveRequest(direction)` como intenção;
- `Error/Disconnect`;
- cliente headless nativo;
- cliente visual mínimo próprio para validar mapa e movimento.

## Primeiro play nativo

O caminho que estamos validando é:

```text
fantasy-client / fantasy-client-gui
        ↓
Fantasy Protocol v1 sobre TCP
        ↓
Fantasy Server
        ↓
WorldRuntime
        ↓
FMAP
```

Não há OTBM, TFS runtime, protocolo 10.98, DAT/SPR/OTB ou OTClient nesse caminho.

O mundo de desenvolvimento atual possui 4 chunks e 64 tiles semânticos. O fluxo automatizado entra em `100,100,7`, recebe os chunks e solicita movimento ao Server, que devolve a posição autoritativa.

## Estado das fases

```text
F00  Independent Core Foundation       PASS
F01  Project System                    PASS
F02  Fantasy Map Core                  PASS
F03  Fantasy Map Editor MVP            TECHNICAL PASS
      └─ gate visual interativo Windows pendente
F04  Fantasy Server World Runtime      PASS
F05  Protocol v1 + First Native Play   AUTOMATED TECHNICAL PASS
      └─ fechamento formal depende dos gates visuais reais no Windows
F06  Persistence / Database            PREPARED / NOT STARTED
```

A F06 não deve começar antes do fechamento formal da F03/F05.

## Validação

Build completo local no Windows:

```powershell
./scripts/build-native.ps1 -Configuration Release
```

Primeiro play headless em dois processos:

```powershell
./scripts/run-native-play.ps1 -Configuration Release -Port 17171 -Character "Development Hero"
```

Play visual para validação interativa:

```powershell
./scripts/run-visual-play.ps1 -Configuration Release -Port 17173 -Character "Visual Hero"
```

O roteiro ordenado para Codex fechar os gates Windows está em:

```text
docs/CODEX-F05-WINDOWS.md
```

Depois do fechamento, a F06 já possui planejamento e ordem de execução preparados em:

```text
docs/F06-PERSISTENCE-PLAN.md
docs/CODEX-AFTER-F05.md
```

## Referências temporárias

Os SHAs externos e suas funções estão em `docs/UPSTREAMS.md`. TFS 1.4.2, RME 3.7 e OTClient 10.98 são **REFERENCE ONLY** e não fazem parte do caminho nativo final.

## Próximo marco

Fechar F03/F05 em Windows real com evidência visual e de processo. Somente depois disso a prioridade muda para F06 — persistência, contas/personagens e save/load.
