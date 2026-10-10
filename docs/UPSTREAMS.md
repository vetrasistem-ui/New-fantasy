# Upstreams — References and approved dependencies

Este documento registra projetos externos usados como baseline, referência técnica, comparação de comportamento ou dependência explicitamente aprovada.

## TFS 1.4.2 — baseline de runtime atual

- Repository: `otland/forgottenserver`
- Release: `v1.4.2`
- Commit: `31d6e85de2a86fb3f0e36c63509fba75b855b8bd`
- Protocol family: `10.98`
- Role: **runtime/server baseline e fonte de verdade de compatibilidade**
- Status: **ACTIVE BASELINE**

O primeiro gate do Map Engine deve produzir mapas que carreguem no TFS 1.4.2 vanilla sem depender de extensões do BlackTek Server.

## BlackTek MapEditor — principal referência do editor 10.98

- Repository: `Black-Tek/BlackTek-MapEditor`
- Branch: `master`
- Commit pinado para a auditoria: `d429c7a4334774983c652bf21764edb396a03c02`
- Map family: `10.98 / OTBM v3`
- Role: **principal oráculo comportamental para OTBM, DAT/SPR/OTB, brushes, houses, spawns, palettes e Undo/Redo**
- Status: **REFERENCE ONLY — NO SOURCE COPY**

Observação de licença: os arquivos-fonte auditados carregam cabeçalho GPLv3-or-later, enquanto o `LICENSE.rtf` da raiz contém uma EULA antiga e conflitante. Até decisão explícita de licença, o código é usado somente como referência técnica/comportamental. Extensões específicas do BlackTek, como Zone IDs/TOML e attribute-map 128, não entram automaticamente na baseline TFS 1.4.2.

## RME 3.7 — referência secundária

- Repository: `hampusborgos/rme`
- Release: `v3.7`
- Commit: `6aceb3c6a311e6e1c0b24a0bf06cf383fb152766`
- Legacy map family: `10.98 / OTBM v3`
- Role: referência histórica/secundária de comportamento e comparação
- Status: **REFERENCE ONLY**

## Client 10.98 reference

- Repository: `opentibiabr/otclient`
- Commit: `396f0b396741bdd4469f27cf9376103930712cff`
- Known compatibility: `TFS 1.4.2 / 10.98`
- Role: client/protocol behavior oracle e base candidata para homologação
- Status: **REFERENCE / CLIENT CANDIDATE**

## SDL3 — approved visual/platform dependency

- Repository: `libsdl-org/SDL`
- Release: `release-3.4.18`
- Commit: `829a65d769d935c4852f8159e964312c0957260a`
- License: zlib
- Role: window/input/platform layer do Fantasy Studio
- Status: **APPROVED DEPENDENCY**

## Dear ImGui — approved Studio UI dependency

- Repository: `ocornut/imgui`
- Release: `v1.92.9b`
- Commit: `f1cc2ae15e53a861a874c3034aae6798fde194ab`
- License: MIT
- Role: Studio editor panels/tooling UI
- Status: **APPROVED DEPENDENCY**

## pugixml — approved legacy XML compatibility dependency

- Repository: `zeux/pugixml`
- Release: `v1.16`
- Published: 2026-06-16
- License: MIT
- Role: leitura/escrita robusta dos arquivos XML externos de houses/spawns usados pela baseline TFS 1.4.2/10.98
- Boundary: somente a camada `Shared/Formats/Legacy`; o `Fantasy Map Core` permanece independente de XML
- Status: **APPROVED DEPENDENCY**

## Cinzel — Studio V5 branding font

- Source: Google Fonts / `google/fonts`, `ofl/cinzel`
- License: SIL Open Font License 1.1
- File: `Studio/UI/Assets/Fonts/Cinzel.ttf`
- Font SHA-256: `f4d83d34d1f6c741193e4acf4b3dff9531e5a67b6aa65228d00a7db72a4e0f34`
- License SHA-256: `f2b3029aba64c378bf0963b62945eee15e564fe4330b934c8f2eb058282b5e83`
- Role: Fantasy Studio V5 branding
- Status: **PRESENTATION ASSET**

## Boundary rule

Reference-only clones podem viver localmente em `.upstream/` e permanecem ignorados pelo Git.

Não copiar source de BlackTek/RME para `Studio/`, `Shared/`, `Server/` ou `Client/` sem decisão explícita de licença e proveniência. Implementações próprias devem manter rastreabilidade de comportamento estudado.

O caminho operacional atual é:

```text
Fantasy Studio V5
      ↓
OTBM v3 + DAT/SPR/OTB + houses/spawns XML
      ↓
TFS 1.4.2
      ↓
Protocol 10.98
      ↓
Client 10.98
```

FMAP, Fantasy Server e Fantasy Protocol próprios permanecem preservados como experimentos técnicos anteriores, mas não definem a baseline operacional atual.

## Update rule

Changing a pinned reference/dependency SHA requires:

1. documented reason;
2. comparison/build rerun if relevant;
3. update to this file;
4. architecture decision update if the change affects the active baseline.