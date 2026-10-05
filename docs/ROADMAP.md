# Roadmap — New Fantasy

## F00 — Independent Core Foundation

Status: **IN_PROGRESS**

Objetivo: congelar os contratos próprios antes de construir sistemas grandes.

Gates:

- layout limpo e caminhos relativos validados;
- `Shared/Protocol/` criado com Fantasy Protocol v1 mínimo;
- `Shared/Formats/FMAP/` criado com FMAP v0 experimental;
- `Game/Maps/World/world.fmap.json` válido;
- Fantasy Server skeleton compila e executa;
- referências 10.98 ficam isoladas em `.upstream/`;
- nenhuma dependência de OTBM/TFS entra no caminho nativo.

## F01 — Project System

- `fantasy.project.json` como manifesto real;
- New/Open/Recent Project;
- caminhos relativos;
- Open Main Map direto;
- mover projeto para outro disco e reabrir.

## F02 — Fantasy Map Core

- estruturas World / Region / Chunk / Tile / Object;
- leitura e escrita FMAP;
- semantic asset keys;
- validação;
- transactions/undo;
- fixtures determinísticas.

## F03 — Fantasy Map Editor MVP

- viewport 2D;
- floors;
- seleção;
- ground/object brush;
- fill/erase;
- minimap básico;
- Save/Reopen FMAP;
- GUI usa as mesmas operações expostas à automação.

## F04 — Fantasy Server World Runtime

- carregar FMAP diretamente;
- world/chunk registry;
- player/entity model mínimo;
- walkability;
- scheduler básico;
- Start/Stop limpo.

## F05 — Fantasy Protocol v1 + First Native Play

- framing/version handshake;
- auth de desenvolvimento;
- enter world;
- map chunk;
- add/move/remove entity;
- Fantasy Client mínimo conecta;
- personagem entra em FMAP e anda.

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
