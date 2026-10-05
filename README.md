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
- `FMAP` é o formato de mapa nativo em desenvolvimento; OTBM é apenas referência/importação legada.
- `Fantasy Protocol` é o protocolo nativo em desenvolvimento; 10.98 é somente ponte/reference adapter temporário.
- `Fantasy Server` será implementado como servidor próprio e autoritativo.
- Studio, CLI, scripts e Codex usam as mesmas operações de domínio.
- Operações de IA devem ser validáveis, determinísticas e reversíveis.
- Código externo não entra silenciosamente no produto; referências ficam isoladas em `.upstream/`.
- O jogo e o Studio crescem juntos depois do primeiro ciclo jogável nativo.

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
│   └── Formats/
├── Database/
├── Tools/
├── Projects/
├── scripts/
└── docs/
```

## Formatos próprios

- **FMAP**: fonte de mapa semântica, versionável, organizada por regiões/chunks e amigável para IA.
- **FMAPC**: formato compilado/runtime futuro, otimizado para carregamento pelo Fantasy Server.
- **Fantasy Protocol v1**: contrato versionado entre Fantasy Server e Fantasy Client.
- **Fantasy Data Model**: contratos compartilhados para entidades, itens, criaturas, mapas e conteúdo.

## Referências temporárias

Os SHAs usados como referência estão em `docs/UPSTREAMS.md`:

- TFS 1.4.2 / protocolo 10.98;
- RME 3.7 / OTBM 10.98;
- OpenTibiaBR OTClient compatível com TFS 1.4.2.

Esses projetos ajudam a comparar comportamento e validar conceitos, mas não são mais a fundação obrigatória do produto final.

## Primeiro grande marco

```text
Open Project
   ↓
abrir FMAP
   ↓
Fantasy Server inicia
   ↓
Fantasy Client conecta pelo Fantasy Protocol
   ↓
personagem entra
   ↓
movimento funciona
```

Esse marco deve funcionar **sem OTBM e sem protocolo 10.98 no caminho principal**.

## Estado

**F00 — INDEPENDENT CORE FOUNDATION: IN_PROGRESS**

A F00 agora define e prova os contratos próprios: layout, FMAP inicial, Fantasy Protocol v1 e esqueleto compilável do Fantasy Server. A stack 10.98 continua apenas como referência controlada.