# Studio

Código do Fantasy Studio.

```text
Studio/
├── Core/
├── MapEngine/
├── Project/
├── Runtime/
├── Automation/
└── UI/
```

O Studio não nasce mais de uma cópia do RME. O `MapEngine/` será uma implementação própria baseada no Fantasy Map Model/FMAP.

RME 3.7 permanece em `.upstream/` apenas para comparação de comportamento, fixtures e eventual interoperabilidade legada.

Princípios:

- GUI e Codex usam o mesmo Map/Core API;
- FMAP é a fonte de verdade do mapa;
- operações grandes são reversíveis;
- nenhuma ferramenta procura arquivos em locais arbitrários;
- `fantasy.project.json` define os caminhos oficiais;
- OTBM/10.98 não entram no caminho nativo do Studio.
