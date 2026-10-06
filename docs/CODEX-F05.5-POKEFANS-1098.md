# CODEX — F05.5 TFS 1.4.2 / PokeFans 10.98 Map Engine Homologation

Status: **READY TO EXECUTE LOCALLY / REALIGNED**

Branch:

```text
feature/f05.5-pokefans-1098
```

## Missão

Homologar o Fantasy Studio V5 como editor real para **TFS 1.4.2 / protocolo 10.98**.

O resultado desta fase deve provar:

```text
OTBM 10.98 real
   ↓ open/render/edit/save
Fantasy Studio V5
   ↓
OTBM v3 gerado + houses/spawns
   ↓
TFS 1.4.2 vanilla
   ↓
Client 10.98
```

Não iniciar F06.

## Fontes de verdade

1. Runtime: `otland/forgottenserver@31d6e85de2a86fb3f0e36c63509fba75b855b8bd`
2. Map Editor behavior oracle: `Black-Tek/BlackTek-MapEditor@d429c7a4334774983c652bf21764edb396a03c02`
3. RME 3.7: referência secundária.
4. Documentos locais:
   - `docs/BLACKTEK-MAPEDITOR-1098-AUDIT.md`
   - `docs/F05.5-LEGACY-ASSET-BRIDGE.md`
   - `docs/UPSTREAMS.md`
   - `AGENTS.md`

## Restrições permanentes

- Crystal/15.24 está fora do projeto.
- Não copiar código BlackTek/RME para o core.
- Não versionar DAT/SPR/OTB/OTBM de terceiros sem autorização.
- Não sobrescrever o OTBM source durante inspeção/desenvolvimento.
- Não introduzir Zone TOML/Zone IDs, OTBM attribute-map 128 ou live-map protocol no primeiro gate.
- SDL3 + SDL_Renderer3 + Dear ImGui continuam obrigatórios para o Studio V5.
- Paths persistidos devem ser relativos.
- Unknown/unsupported nunca pode ser descartado silenciosamente.
- O TFS 1.4.2 vanilla decide se o OTBM salvo é compatível.
- FMAP não é requisito desta fase; não converter OTBM para FMAP para cumprir o gate.

## C00 — sincronização e baseline

```powershell
git fetch origin
git checkout feature/f05.5-pokefans-1098
git pull --ff-only origin feature/f05.5-pokefans-1098
git status --short
git rev-parse HEAD
./scripts/build-native.ps1 -Configuration Release
```

Registrar SHA inicial e regressões existentes.

## C01 — pack 10.98 exato

Localizar e hash:

```text
Tibia.dat
Tibia.spr
items.otb
*.otbm
*-house.xml
*-spawn.xml
```

Usar somente um conjunto coerente. Registrar tamanho, SHA-256 e signatures.

## C02 — differential asset readers

Validar readers próprios existentes:

- `DatReader`
- `SprReader`
- `OtbReader`
- `LegacyAssetRegistry`

Fluxo:

```text
OTB serverId -> clientId
DAT clientId -> appearance/frame/sprite ids
SPR sprite id -> pixels
```

Comparar amostras e contagens contra o comportamento observado no BlackTek MapEditor.

Diagnosticar:

- OTB sem DAT;
- DAT sem sprite;
- sprite inválida;
- duplicatas;
- signatures incompatíveis.

## C03 — differential OTBM reader

Usar `OtbmReader` existente e ampliar somente onde necessário.

Comparar, para o mesmo mapa:

- width/height;
- OTBM header/version;
- floors;
- tile count;
- item count;
- server ids;
- tile flags;
- houses;
- towns;
- waypoints;
- attributes;
- external house/spawn filenames.

A referência de comportamento é BlackTek/RME, mas a compatibilidade final é TFS 1.4.2.

## C04 — houses e spawns

Implementar/levar ao modelo de trabalho:

- `map-house.xml`;
- `map-spawn.xml`;
- house ids;
- house tiles;
- house exits quando aplicável;
- towns/temple positions;
- spawn centers/radius;
- monsters/NPCs e direção/spawntime quando presentes.

Unknown XML fields devem entrar em diagnostics.

## C05 — render 10.98 real no Studio V5

Integrar asset registry ao renderer sem alterar a identidade visual V5.

Requisitos:

- lazy sprite decoding/cache;
- SDL_Renderer3 textures;
- stack order correto;
- ground/borders/objects reais;
- floor navigation;
- XYZ;
- minimap;
- picker/inspector exibindo serverId/clientId/metadata relevantes;
- placeholder determinístico para asset ausente.

Não colocar `SDL_Texture*` em modelos persistentes.

## C06 — ferramentas sobre mapa real

Implementar/homologar progressivamente:

- Select/Picker;
- raw/object placement;
- ground paint;
- Erase;
- brush footprint preview;
- square/circle sizes;
- copy/paste;
- Undo/Redo por gesto;
- floors;
- minimap navigation.

Autoborder/autowall só entra quando as regras 10.98 estiverem comparadas contra BlackTek/RME e cobertas por teste.

## C07 — writer OTBM v3 próprio

Criar writer próprio para o subconjunto aceito pelo TFS 1.4.2.

Primeiro escopo permitido:

- identifier OTBM;
- header version 2 (OTBM v3 na nomenclatura do editor);
- width/height;
- item major/minor compatíveis;
- MAP_DATA;
- DESCRIPTION;
- EXT_SPAWN_FILE;
- EXT_HOUSE_FILE;
- TILE_AREA;
- TILE/HOUSETILE;
- TILE_FLAGS;
- compact item e item nodes;
- atributos padrão necessários;
- TOWNS;
- WAYPOINTS quando presentes.

Não emitir no primeiro gate:

- attribute-map 128;
- Zone IDs/TOML;
- nodes custom BlackTek;
- atributos desconhecidos inventados.

## C08 — writer houses/spawns

Gerar XML auxiliar compatível com TFS 1.4.2 e manter nomes relativos ao mapa.

Não sobrescrever source; salvar em pasta/output de teste.

## C09 — Save/Reopen Fantasy

Executar roundtrip:

```text
source OTBM
   ↓
open
   ↓
edit determinístico
   ↓
save as novo OTBM
   ↓
reopen no Fantasy
```

Comparar semanticamente:

- posições;
- floors;
- item ids/order;
- tile flags;
- houses;
- towns;
- waypoints;
- spawn/house references;
- atributos suportados.

## C10 — BlackTek/RME comparison

Abrir o OTBM gerado no BlackTek/RME para verificação visual e estrutural.

Isso é comparação, não gate final.

Registrar qualquer diferença.

## C11 — TFS 1.4.2 hard gate

Compilar/executar TFS 1.4.2 pinado e apontar para o mapa gerado.

O gate falha se o TFS reportar:

- unknown OTBM version;
- unknown node;
- unknown header/tile/item attribute;
- incompatible items.otb;
- house/spawn inválido essencial;
- crash/assert;
- corrupção de mapa.

TFS load limpo é obrigatório.

## C12 — Client 10.98 play gate

Conectar cliente 10.98 ao TFS 1.4.2 real:

- login;
- character list quando aplicável;
- enter world;
- render do mapa editado;
- movimento;
- reconnect;
- disconnect/shutdown limpos.

## C13 — edição de homologação

No mapa real executar pelo menos:

1. selecionar tile/item;
2. pintar ground;
3. colocar object;
4. apagar;
5. Undo;
6. Redo;
7. Save As;
8. reabrir;
9. carregar no TFS;
10. entrar pelo client 10.98.

O resultado precisa sobreviver de ponta a ponta.

## C14 — evidência e fechamento

Registrar:

```text
docs/evidence/F05.5/
```

com:

- SHA inicial/final;
- hashes do source pack;
- versões/signatures;
- contagens DAT/SPR/OTB/OTBM;
- diagnostics;
- screenshots Studio;
- logs de TFS load;
- evidência Client 10.98;
- matriz de preservação de atributos;
- confirmação de que source original permaneceu intacto.

Rodar:

```powershell
./scripts/build-native.ps1 -Configuration Release
git diff --check
git status --short
```

## PASS

F05.5 somente pode ser marcada PASS quando:

- readers reais 10.98 passam;
- mapa real abre e renderiza;
- edição + Undo/Redo funcionam;
- writer OTBM v3 funciona;
- Save/Reopen Fantasy passa;
- BlackTek/RME consegue abrir para comparação;
- **TFS 1.4.2 vanilla carrega o mapa salvo**;
- **Client 10.98 entra e movimenta no mapa salvo**;
- nenhuma extensão BlackTek é requisito oculto;
- nenhum binário de terceiros foi commitado sem autorização;
- F06 permanece NOT STARTED.

Continue sozinho até um bloqueio real de ambiente local, incompatibilidade de dados ou decisão do owner. Não trocar a arquitetura por conveniência.
