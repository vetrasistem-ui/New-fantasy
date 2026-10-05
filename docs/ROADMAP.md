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

Status: **PASS**

Implementado e validado automaticamente:

- SDL3 + SDL_Renderer3 + Dear ImGui (ADR-019);
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

A sessão visual real no Windows passou em 2026-10-05: quatro chunks, seleção,
Paint, Undo/Redo, Fill entre chunks, Add/Remove Object, Erase e Save/Reopen com
persistência confirmada e screenshots reais. O FMAP canônico foi restaurado.

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

Status: **PASS**

Objetivo: ligar Fantasy Server e Fantasy Client pelo protocolo próprio até um personagem entrar no FMAP e andar.

Implementado e comprovado automaticamente no Windows:

- codec binário do envelope v1;
- framing TCP real;
- `Hello` / `HelloAck` e version handshake;
- `LoginDev` / `LoginOk`;
- `EnterWorld`;
- `MapChunk` + FMCP v1;
- `EntityAdd` / `EntityMove` / `EntityRemove`;
- Fantasy Client headless conecta ao Fantasy Server;
- Fantasy Client GUI mínimo usa o mesmo core;
- movimento é solicitado pelo Client e validado/autorizado pelo Server;
- personagem entra no mapa FMAP e anda sem OTBM e sem protocolo 10.98;
- disconnect/reconnect básico e encerramento limpo;
- integração TCP loopback real;
- primeiro play com Server e Client em processos separados;
- pacote Windows de Studio e runtime nativo.

O roteiro C00–C07 de `docs/CODEX-F05-WINDOWS.md` passou em 2026-10-05: Studio
visual, Client visual, movimento autoritativo e rejeição no limite, reconnect,
cleanup, screenshots reais e regressão completa. Evidência detalhada:
`docs/evidence/F05/WINDOWS-VALIDATION.md`.

**Primeiro grande marco concluído: primeiro play nativo visual confirmado sem
OTBM e sem protocolo 10.98.**

Evidência: `docs/evidence/F05/RESULT.md`.

## F05.5 — Legacy Asset Bridge (DAT / SPR / OTB)

Status: **PREPARED / NOT STARTED**

Objetivo: colocar sprites reais no Map Editor e no Client agora, reutilizando
DAT + SPR + OTB como fonte de assets sem recriar todo o conteúdo visual do zero.

Regras:

- DAT/SPR/OTB entram por um `Legacy Asset Bridge` isolado;
- FMAP continua usando chaves semânticas Fantasy;
- Studio e Client compartilham o mesmo `Fantasy Asset Registry`;
- IDs/caminhos legados não viram contrato de domínio;
- arquivos binários legados são fornecidos pelo projeto/usuário e não são
  redistribuídos no repositório quando não houver autorização;
- placeholder determinístico para asset ausente;
- Save/Reopen não serializa textura/cache no FMAP.

Gates MVP:

- abrir/validar DAT + SPR + OTB;
- resolver ground, border e object;
- renderizar sprites reais no Studio;
- palette mínima para ground/object;
- mesmo registry no Client;
- validação visual Windows;
- regressões F00–F05 verdes.

Plano detalhado: `docs/F05.5-LEGACY-ASSET-BRIDGE.md`.

## F06 — Persistence / Database

Status: **PREPARED / NOT STARTED**

F03/F05 passaram. Após o bootstrap visual F05.5, F06 continua sendo a próxima
fase estrutural de persistência; sua implementação permanece NOT STARTED.

Plano preparado:

- `PersistenceService` / store interface independente de SQL;
- primeiro adapter SQLite para desenvolvimento/CI determinístico;
- migrations append-only;
- contas locais de desenvolvimento;
- personagens;
- save/load de posição autoritativa;
- restart/reconnect persistence validation;
- sem public auth, inventário, skills ou website nesta fase.

Plano detalhado: `docs/F06-PERSISTENCE-PLAN.md`.

Execução Codex após F05: `docs/CODEX-AFTER-F05.md`.

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

- Fantasy Asset Registry completo;
- sprites/effects/UI próprios;
- semantic keys;
- import/migration tools para referências legadas;
- atlas, animações, efeitos e substituição gradual dos assets bootstrap.

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

## F17 — Website / Accounts

- web account flow;
- integração com auth/persistence pública;
- status/downloads.

## F18 — Build / Distribution

- instaladores;
- packages;
- versionamento;
- update/distribution.

## F19 — Publish / VPS

- deploy de servidor;
- operação 24/7;
- logs/backup/observabilidade;
- processo de publicação.
