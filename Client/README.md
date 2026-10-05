# Client

Cliente oficial do Fantasy.

```text
Client/
├── Core/
├── Modules/
├── UI/
└── Assets/
```

O alvo final é um **Fantasy Client** falando `Fantasy Protocol`, com UI, assets e sistemas próprios.

Um OTClient 10.98 pode ser usado em `.upstream/` como referência temporária de comportamento durante a migração, mas não define o contrato final nem deve ser copiado automaticamente para `Client/Core/`.

O primeiro cliente nativo precisa apenas de:

- handshake;
- login de desenvolvimento;
- enter world;
- receber um chunk de mapa;
- adicionar/mover/remover entidades;
- enviar intenção de movimento.

A modernização visual vem depois do primeiro ciclo nativo jogável.
