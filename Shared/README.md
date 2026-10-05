# Shared

Contratos canônicos compartilhados entre Studio, Server e Client.

```text
Shared/
├── Protocol/       # Fantasy Protocol
└── Formats/
    └── FMAP/       # Fantasy Map Format
```

Regras:

- uma definição canônica por contrato;
- Server/Client/Studio não mantêm cópias divergentes;
- mudanças incompatíveis exigem nova versão ou migração explícita;
- formatos devem ser simples de validar e manipular por ferramentas/IA.
