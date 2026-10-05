# Roadmap — New Fantasy

## F00 — Independent Core Foundation

Status: **PASS**

Objetivo: congelar os contratos próprios antes de construir sistemas grandes.

Gates concluídos:

- layout limpo e caminhos relativos validados;
- `Shared/Protocol/` criado com Fantasy Protocol v1 mínimo;
- `Shared/Formats/FMAP/` criado com FMAP v0 experimental;
- `Game/Maps/World/world.fmap.json` validado estrutural e semanticamente;
- IDs, nomes, tipos e direções do Fantasy Protocol validados automaticamente;
- Fantasy Server skeleton compila e executa como C++20;
- referências 10.98 ficam isoladas em `.upstream/`;
- nenhuma dependência de OTBM/TFS entra no caminho nativo.

Evidência: `docs/evidence/F00/RESULT.md`.

## F01 — Project System

Status: **PASS**

- `fantasy.project.json` como manifesto real;
- schema/validator do manifesto;
- resolução central de caminhos relativos;
- New/Open/Recent Project;
- Open Main Map direto;
- nenhum módulo procura recursos principais manualmente;
- mover projeto para outro disco/pasta e reabrir sem alteração persistida.

Evidência: `docs/evidence/F01/RESULT.md`.

## F02 — Fantasy Map Core

Status: **PASS**

- estruturas World / Region / Chunk / Tile / Object;
- leitura e escrita FMAP;
- semantic asset keys;
- validação;
- transactions/undo/redo/rollback;
- fixtures determinísticas e roundtrip semântico.

Evidência: `docs/evidence/F02/RESULT.md`.

## F03 — Fantasy Map Editor MVP

Status: **TECHNICAL PASS / INTERACTIVE WINDOWS VISUAL CHECK PENDING**

Implementado e validado automaticamente:

- SDL3 + SDL_GPU + Dear ImGui;
- janela/editor nativo;
- viewport 2D;
- floors;
- pan/zoom e seleção;
- ground/object brush;
- Fill e Erase pela camada compartilhada de operações;
- Undo/Redo;
- minimapa básico;
- Save/Reopen FMAP em roundtrip semântico;
- mundo de desenvolvimento 8 x 8 / 64 tiles / quatro chunks;
- semântica multi-chunk e Fill atravessando fronteira de chunk.

O único gate ainda não executado é a sessão visual interativa no Windows com captura de evidência. Esse gate não bloqueia o runtime/protocolo nativo, mas permanece obrigatório antes do fechamento visual definitivo da F03.

Evidência: `docs/evidence/F03/RESULT.md`.

## F04 — Fantasy Server World Runtime

Status: **PASS**

Gates concluídos:

- carregamento direto de `world.fmap.json`;
- FMAP C++ neutro extraído para `Shared/Formats/FMAP/` e consumido por Studio + Server;
- índice global Region / Chunk / Tile;
- detecção de coordenadas resolvidas duplicadas;
- entity/player model mínimo;
- walkability autoritativa para tile ausente/bloqueado;
- ocupação exclusiva de tile;
- movimento cardinal e travessia entre chunks;
- ciclo Start / Ready / Tick / Stop;
- scheduler determinístico com atraso/cancelamento;
- smoke test e testes headless do runtime;
- regressões F00–F03 verdes no workflow Windows.

Evidência: `docs/evidence/F04/RESULT.md`.

## F05 — Fantasy Protocol v1 + First Native Play

Status: **IN_PROGRESS**

Objetivo: ligar Fantasy Server e Fantasy Client pelo protocolo próprio até um personagem entrar no FMAP e andar.

Gates:

- codec binário do envelope v1;
- framing TCP;
- `Hello` / `HelloAck` e version handshake;
- `LoginDev` / `LoginOk`;
- `EnterWorld`;
- `MapChunk`;
- `EntityAdd` / `EntityMove` / `EntityRemove`;
- Fantasy Client mínimo conecta ao Fantasy Server;
- movimento é solicitado pelo Client e validado/autorizado pelo Server;
- personagem entra no mapa FMAP e anda sem OTBM e sem protocolo 10.98;
- disconnect/reconnect básico e encerramento limpo;
- testes de codec, framing e sessão de desenvolvimento.

**Primeiro grande marco: F05 PASS sem OTBM e sem protocolo 10.98 no caminho principal.**

## F06 — Persistence / Database

- contas e personagens;
- save/load de posição;
- migrations;
- reconnect/restart validation.

## F07 — Item / Inventory Core

- item definitions;
- inventory;
- containers;
- semantic IDs;
- Item Editor inicial.

## F08 — Creature / Monster Core

- creatures;
- AI básica;
- spawn;
- Monster Editor;
- loot.

## F09 — Combat / Spell / Skill

- combate autoritativo;
- cooldown;
- effects/events;
- Spell/Skill Editor.

## F10 — NPC / Dialogue

- NPCs;
- shops;
- dialogue model;
- conditions/actions.

## F11 — Quest System

- etapas;
- dependências;
- rewards;
- Quest Editor e validator.

## F12 — Classes / Attributes / Equipment

- atributos próprios;
- level requirements;
- attribute requirements;
- equipment bonuses;
- passives/classes.

## F13 — Asset Pipeline

- Fantasy Asset Registry;
- sprites/effects/UI próprios;
- semantic keys;
- import tools temporárias para referências legadas quando necessário.

## F14 — Automation API / Codex

- CLI estável;
- map/content commands;
- templates;
- generation by regions;
- transactions/preview/undo;
- validators.

## F15 — System Lab

Modo Visual / Híbrido / Código para sistemas customizados.

## F16 — Client Modernization

- UI própria;
- particles/effects;
- lighting/shaders quando necessário;
- launcher/updater;
- remover adapters legados restantes.

## F17 — Website / Account Platform

Somente após runtime nativo local estar estável.

## F18 — Build / Distribution

- Studio;
- Server;
- Client;
- assets;
- updater;
- pacote reproduzível.

## F19 — Publish / VPS

Deploy 24/7, observabilidade, backups, atualização segura.

## Regra de transição

TFS 1.4.2, RME 3.7 e cliente 10.98 são usados apenas para comparação e fallback. A cada sistema nativo validado, reduzimos a dependência do legado até que o caminho final seja somente:

```text
Fantasy Studio → FMAP → Fantasy Server → Fantasy Protocol → Fantasy Client
```
