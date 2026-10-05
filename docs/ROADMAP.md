# Roadmap — New Fantasy

## F00 — Foundation & Compatibility

Status: **IN_PROGRESS**

Objetivo: provar que todas as bases candidatas falam a mesma linguagem antes de transformar o editor em Studio.

Gates:

- TFS 1.4.2 compila e inicia;
- MariaDB importa schema e persiste conta/personagem;
- OTClient conecta em 10.98 e chega ao jogo;
- RME abre mapa 10.98;
- RME edita, salva e reabre sem corrupção;
- TFS carrega o mapa salvo;
- cliente visualiza a alteração;
- asset mínimo é reconhecido de forma consistente;
- SHAs e versões são congelados;
- licenças das bases incorporadas são revisadas.

## F01 — Fantasy Identity

- renomear produto para Fantasy Studio;
- título, splash, ícone e identidade;
- remover referências visuais desnecessárias do editor original;
- preservar o Map Engine sem mudanças funcionais desnecessárias.

## F02 — Project System & Clean Layout

- `fantasy.project.json` real;
- New Project / Open Project / Recent Projects;
- caminhos relativos;
- defaults conhecidos;
- `Open Main Map` sem diálogo de procura;
- mover a pasta inteira e reabrir com sucesso.

## F03 — Server Manager

- Start / Stop / Restart;
- console integrado;
- logs;
- detecção de processo órfão;
- config do servidor por projeto.

## F04 — Database Manager

- conectar MariaDB;
- migrar schema;
- seed de conta/personagem de teste;
- status READY/ERROR no Studio.

## F05 — Client Manager + First Play Gate

- configurar endpoint do projeto;
- abrir cliente;
- botão `Play`;
- `Play` valida banco, inicia servidor e abre cliente;
- personagem entra em `Game/Maps/world.otbm`.

**Primeiro grande marco: F05 PASS.**

## F06 — Item Editor

Modelo Fantasy + adapter 10.98/TFS.

## F07 — Monster Editor

Monstros, loot, ataques, resistências e validação.

## F08 — Spawn Editor

Integração mapa + criaturas + áreas.

## F09 — Spell / Skill Editor

Dados simples primeiro; visualização avançada depois.

## F10 — NPC / Dialogue Editor

Diálogo, lojas, condições e integração com quests.

## F11 — Quest Editor

Etapas, condições, rewards e dependency validation.

## F12 — Class / Attribute Editor

Base para sistemas próprios do jogo.

## F13 — Asset Manager

Gerenciamento de DAT/SPR/OTB e catálogo semântico Fantasy.

## F14 — Automation API / Codex

- CLI estável;
- mapa por comandos/scripts;
- transactions/undo para alterações de IA;
- validators;
- templates e geração procedural.

## F15 — System Lab

Modo Visual / Híbrido / Código para sistemas customizados.

## F16 — Website / Account Integration

Somente após runtime local estar estável.

## F17 — Build / Distribution

Pacote do Studio, cliente e runtime reproduzível.

## F18 — Publish / VPS

Deploy e operação 24/7.

## Desenvolvimento do jogo

O jogo cresce junto com o Studio após F05. Cada editor deve nascer para resolver uma necessidade real de conteúdo, evitando construir ferramentas sem uso comprovado.
