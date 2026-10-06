# CODEX — F05.5 PokeFans 10.98 / OTBM + Real Sprites

Status: **READY TO EXECUTE LOCALLY**

Branch:

```text
feature/f05.5-pokefans-1098
```

## Missão

Conectar o pack local PokeFans 10.98 ao Fantasy Studio V5 e chegar ao primeiro mapa OTBM grande renderizado com sprites reais.

Não iniciar F06.

## Restrições permanentes

- não copiar código de RME/TFS/PokeFans para o core;
- não versionar DAT/SPR/OTB/OTBM binários;
- não salvar por cima do OTBM de origem;
- não colocar parser legado em `MapDocument`, FMAP, Server ou Protocol;
- não criar segundo modelo nativo de mapa;
- SDL3 + SDL_Renderer3 + Dear ImGui continuam obrigatórios;
- caminhos locais devem ser resolvidos por profile/config relativo, nunca persistidos como absolutos;
- qualquer item/atributo não suportado deve aparecer em diagnóstico; não descartar silenciosamente.

## C00 — sincronização e baseline

```powershell
git fetch origin
git checkout feature/f05.5-pokefans-1098
git pull --ff-only origin feature/f05.5-pokefans-1098
git status --short
git rev-parse HEAD
./scripts/build-native.ps1 -Configuration Release
```

Registrar SHA inicial e confirmar regressões verdes antes de tocar no bridge.

## C01 — descobrir o pack real, sem alterar arquivos

Localizar os arquivos fornecidos pelo owner. Esperado encontrar equivalentes a:

```text
global_dash.otbm
Tibia.dat
Tibia.spr
items.otb
map-house.xml
map-spawn.xml
```

Há mais de um DAT/SPR/OTB no material local. NÃO misturar conjuntos.

Para cada candidato registrar:

- caminho relativo/local de trabalho;
- tamanho;
- SHA-256;
- versão/signature observável;
- qual OTB foi usado com o mapa no RME de referência.

Manter o source pack original somente leitura quando possível.

O mapa conhecido apresentou compatibilidade 10.98 / OTB item major 3 minor 57 durante a inspeção anterior. Revalidar no arquivo real; não assumir somente pelo nome da pasta.

## C02 — profile explícito

Usar `Shared/Assets/LegacyAssetRegistry.*` como contrato inicial.

Criar carregamento de profile/config para:

```text
id: pokefans1098
family: PokeFans
clientVersion: 10.98
datPath
sprPath
otbPath
mapPath
housesPath (opcional)
spawnsPath (opcional)
```

O profile deve aceitar nomes físicos diferentes.

Falhar claramente se DAT/SPR/OTB estiver faltando ou se o path fugir da política esperada.

## C03 — SPR 10.98

Implementar reader isolado, com testes.

Requisitos:

- validar signature/header observável;
- indexar sprites sem decodificar tudo antecipadamente;
- decodificar uma sprite por id para RGBA;
- bounds checks rigorosos;
- erro determinístico para id inválido/truncado;
- fixture mínima e legalmente segura criada pelo teste, sem copiar sprite de terceiros para Git.

Não integrar UI antes dos testes do reader passarem.

## C04 — DAT + OTB

Implementar metadata readers/mapping isolados.

Produzir registros equivalentes a:

```text
serverId -> clientId -> sprite ids / appearance metadata
```

Popular `FantasyAssetRegistry`.

Para itens sem alias Fantasy amigável usar:

```text
legacy.pokefans1098.item.<serverId>
```

Gerar relatório:

- total de entries OTB;
- total mapeado para DAT;
- client IDs ausentes;
- sprite IDs inválidos;
- duplicatas/conflitos.

## C05 — textura/cache no Studio

Criar adaptador de apresentação separado do registry de domínio.

Requisitos:

- SDL_Renderer3 textures;
- lazy load/cache;
- liberação determinística de textura;
- placeholder seguro para asset ausente;
- não guardar `SDL_Texture*` dentro de FMAP/MapDocument/registry persistente.

Primeiro gate visual: renderizar um pequeno conjunto conhecido de ground/object real dentro de Items & Assets.

## C06 — OTBM parser para modelo neutro

Implementar parser OTBM somente na camada legado/import.

Saída obrigatória:

```text
LegacyMapImportModel
```

Não mutar `MapDocument` durante parsing.

Preservar no modelo, quando presentes:

- width/height;
- x/y/z;
- ground/item server ids;
- item attributes reconhecidos;
- towns/temple positions;
- house ids/entry metadata;
- external houses XML;
- external spawns XML;
- textos/action/unique IDs e demais atributos relevantes quando encontrados.

Se algo não for suportado, registrar warning/diagnóstico com contagem e contexto.

## C07 — abrir `global_dash.otbm`

Abrir o mapa real local com o profile exato.

Antes:

```powershell
Get-FileHash <map> -Algorithm SHA256
```

Depois da sessão repetir o hash. Deve ser idêntico.

Produzir relatório determinístico:

```text
map dimensions
floors observados
tiles
items
unique server ids
unknown server ids
towns
houses
spawns
unsupported attributes
```

Não tente ainda converter tudo para FMAP se a leitura não estiver loss-aware.

## C08 — mapa real no Studio

Adicionar modo/entrada explícita de compatibilidade legado no Studio, sem mudar a identidade V5.

Objetivo visual:

- viewport mostra o OTBM real;
- grounds/borders/objects usam sprites reais;
- floors e coordenadas funcionam;
- Minimap usa o mapa importado;
- seleção/picker identifica item/ground legado e semantic key bootstrap;
- Items & Assets mostra sprites reais resolvidos pelo mesmo registry.

O mapa aberto pode permanecer em modelo de import temporário durante este gate. Não fingir que já é FMAP.

## C09 — edição mínima sobre o mapa real

Depois do render correto, conectar somente ferramentas essenciais já existentes/contratadas:

- Select/Picker;
- preview de brush;
- Paint simples;
- Object placement simples;
- Undo/Redo por operação.

A edição que será persistida precisa passar pelo core Fantasy/conversão controlada. Não salvar OTBM original.

## C10 — conversão para FMAP

Criar conversor separado:

```text
LegacyMapImportModel -> FMAP/MapDocument
```

Características:

- transacional;
- diagnóstico de perdas;
- unknown IDs preservados por bootstrap semantic keys;
- sem caminhos físicos/sprite offsets no FMAP;
- Save/Reopen semanticamente estável.

Se o FMAP atual não comportar algum dado essencial de houses/spawns/attrs, parar e propor extensão versionada antes de descartar informação.

## C11 — fechamento

Rodar:

```powershell
./scripts/build-native.ps1 -Configuration Release
git diff --check
git status --short
```

Capturar evidência real do Windows:

```text
docs/evidence/F05.5/pokefans-map-real.png
docs/evidence/F05.5/pokefans-items-assets.png
docs/evidence/F05.5/import-report.md
```

Gate mínimo para declarar primeiro milestone PASS:

- source hashes preservados;
- DAT/SPR/OTB exatos validados;
- OTBM abre sem crash;
- mapa grande aparece no Studio;
- ground/border/object real aparece com sprite correto;
- unknowns são reportados;
- build/testes anteriores continuam verdes;
- nenhum binário legado foi commitado;
- F06 continua NOT STARTED.

## Relatório final do Codex

Entregar:

- SHA inicial/final;
- hashes do source pack usado;
- files/classes criados;
- contagens do mapa;
- contagem de assets resolvidos/desconhecidos;
- build/testes;
- screenshots;
- diferenças ainda não suportadas;
- confirmação de source OTBM inalterado;
- confirmação de que F06 não iniciou.

Continue sozinho até o limite em que uma decisão de schema/semântica ou aprovação visual do owner seja realmente necessária.
