# AGENTS.md — New Fantasy

Este arquivo define regras obrigatórias para Codex e outros agentes que trabalhem nesta branch.

## Direção oficial atual

O baseline operacional do New Fantasy é:

```text
Fantasy Studio V5
      ↓
OTBM v3 + DAT/SPR/OTB
      ↓
TFS 1.4.2
      ↓
Protocol 10.98
      ↓
Client 10.98
```

Crystal 4.1.8 / 15.24 não faz parte deste projeto.

BlackTek MapEditor é a principal referência comportamental do Map Editor 10.98. RME 3.7 é referência secundária. TFS 1.4.2 é a fonte de verdade para compatibilidade de runtime.

A auditoria atual está em `docs/BLACKTEK-MAPEDITOR-1098-AUDIT.md`. Quando houver conflito com ADRs anteriores que promoviam FMAP/Fantasy Server/Fantasy Protocol como runtime principal, a direção 1.4.2/10.98 desta branch prevalece até a consolidação formal dos ADRs.

## Regras obrigatórias

1. Não mudar a baseline TFS 1.4.2 / 10.98 sem decisão explícita do owner e documentação correspondente.
2. TFS 1.4.2 pinado (`31d6e85...`) é a fonte de verdade para o que o mapa/runtime precisa aceitar.
3. BlackTek MapEditor pinado (`d429c7a...`) é REFERENCE ONLY e principal oráculo de comportamento do editor 10.98.
4. Não copiar código BlackTek/RME para o core sem decisão explícita de licença e proveniência.
5. O source BlackTek auditado possui cabeçalhos GPLv3-or-later e um `LICENSE.rtf` conflitante; manter isolamento até resolver a estratégia de distribuição.
6. O formato operacional de mapa desta baseline é OTBM v3. FMAP fica preservado como experimento anterior, não como requisito do caminho TFS.
7. DAT/SPR/OTB 10.98 são formatos ativos de compatibilidade, não apenas dados temporários de importação.
8. Não introduzir extensões BlackTek no arquivo salvo sem prova de compatibilidade com TFS 1.4.2 vanilla.
9. Em especial, Zone IDs/TOML, Zone Brush, attribute-map 128 e live-map protocol ficam fora do primeiro gate.
10. OTBM, houses e spawns do pack de origem devem ser tratados como read-only até existir writer homologado; nunca sobrescrever a fonte durante inspeção.
11. Binários/assets do pack local não entram no Git sem autorização explícita de redistribuição.
12. Implementações Fantasy de DAT/SPR/OTB/OTBM devem continuar próprias, testáveis e rastreáveis.
13. Nenhum node/atributo/item desconhecido pode ser descartado silenciosamente; registrar diagnostics com contexto.
14. O writer OTBM inicial deve emitir somente o subconjunto comprovado como aceito por TFS 1.4.2.
15. Todo arquivo salvo pelo Studio precisa passar pelo gate real de carga no TFS 1.4.2.
16. O gate final inclui Client 10.98 entrando e movimentando no mapa salvo pelo Studio.
17. SDL3 + SDL_Renderer3 + Dear ImGui continuam sendo o baseline visual do Fantasy Studio V5.
18. Não substituir a UI V5 por wxWidgets/RME/BlackTek UI.
19. GUI, scripts e automação devem compartilhar as mesmas operações de domínio sempre que possível.
20. Ações de edição devem ser reversíveis; uma ação/gesto significativo deve gerar histórico coerente para Undo/Redo.
21. Preview de brush e mutação devem usar a mesma footprint/regra.
22. Paths persistidos devem ser relativos à raiz do projeto.
23. Não duplicar mapas, assets, server, client ou conteúdo em múltiplas localizações oficiais.
24. Não salvar builds, caches, bancos locais ou logs efêmeros no Git.
25. Compilar sozinho não significa PASS; cada mudança precisa de gate objetivo.
26. F04/F05 do runtime Fantasy próprio permanecem preservadas como experimentos concluídos, mas não validam o runtime TFS atual.
27. Não iniciar uma nova camada de Persistence/F06 antes de homologar o caminho TFS 1.4.2/10.98 ou de receber nova prioridade explícita do owner.

## Estrutura preferida

```text
Studio/
Game/
  Maps/
  Content/
  Assets/
  Imports/
Server/
Client/
Shared/
  Assets/
  Formats/
Tests/
Database/
Tools/
Projects/
docs/
scripts/
```

## Gate mínimo de mudanças 10.98

Se tocar em assets/mapa:

- readers compilam e testes sintéticos passam;
- source local permanece byte-identical em operações read-only;
- unknowns são reportados;
- comparação contra BlackTek/RME é feita quando relevante;
- OTBM salvo pelo Fantasy reabre no Fantasy;
- OTBM salvo pelo Fantasy carrega no TFS 1.4.2 vanilla;
- houses/spawns/towns/waypoints relevantes são preservados;
- regressões da UI V5 permanecem verdes.

Se tocar em runtime 10.98:

- TFS 1.4.2 inicia sem erro do mapa;
- client 10.98 conecta;
- login/enter world funciona;
- movimento básico funciona;
- reconnect/shutdown não deixam processo órfão.

## Prioridade atual

**F05.5 — TFS 1.4.2 / 10.98 Map Compatibility + Real Map Bootstrap: IN PROGRESS.**

Ordem atual:

1. differential readers contra BlackTek/TFS;
2. resolução OTB → DAT → SPR;
3. abrir/renderizar mapa real 10.98;
4. houses/spawns;
5. ferramentas de edição sobre mapa real;
6. writer OTBM v3;
7. Save/Reopen;
8. load no TFS 1.4.2;
9. play com client 10.98;
10. homologação formal do Map Engine.

Documentos principais:

- `docs/BLACKTEK-MAPEDITOR-1098-AUDIT.md`
- `docs/F05.5-LEGACY-ASSET-BRIDGE.md`
- `docs/UPSTREAMS.md`
- `docs/STUDIO-MAP-TOOLS-RME-REFERENCE.md`
