# Upstreams — F00 candidates

Este documento registra as bases externas candidatas. Os SHAs abaixo são pontos reproduzíveis para homologação; ainda não significam aprovação final para incorporação/distribuição.

## Server

- Repository: `otland/forgottenserver`
- Release: `v1.4.2`
- Commit: `31d6e85de2a86fb3f0e36c63509fba75b855b8bd`
- Protocol family: `10.98`
- Status: **CANDIDATE — F00 validation required**

## Map Engine

- Repository: `hampusborgos/rme`
- Release: `v3.7`
- Commit: `6aceb3c6a311e6e1c0b24a0bf06cf383fb152766`
- `data/clients.xml` contains 10.98 as a visible/default client and maps it to OTB 57 with OTBM v3.
- Status: **CANDIDATE — F00 validation + license review required**

## Client

- Repository: `opentibiabr/otclient`
- Candidate commit: `396f0b396741bdd4469f27cf9376103930712cff`
- The upstream compatibility table lists `TFS 1.4.2 (10.98)` as supported.
- Status: **CANDIDATE — F00 validation + license review required**

## Database

- Product family: MariaDB
- Exact version: **TO BE PINNED during F00**
- Requirement: clean schema import, test account seed, persistence and restart validation.

## Asset family

- Client data: DAT + SPR 10.98
- Server item mapping: OTB compatible with the selected TFS/RME combination
- Final asset pack: **TO BE PINNED during F00**

## Rule

Do not update any upstream silently. Changing a pinned SHA requires:

1. documented reason;
2. compatibility rerun;
3. build/test evidence;
4. update to this file;
5. decision entry in `docs/DECISIONS.md` if the change affects the official platform.
