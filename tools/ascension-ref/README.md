# ascension-ref

Referência local dos dados Ascension / Conquest of Azeroth, para corrigir
issues do servidor com dados reais de referência (nome, tooltip, efeitos,
escola, NPCs que usam o spell).

Fonte: [hertigservices/ascension-data](https://github.com/hertigservices/ascension-data)
(manifestos + packs nas Releases + mirror exiles-db). Os caches bulk NÃO
ficam no git — ficam nas Releases e são baixados sob demanda.

**Tudo em `data/` e `index/` é local e NÃO é commitado** (ver `.gitignore`).

## Requisitos

- Python 3 (stdlib apenas)
- CLI [`gh`](https://cli.github.com/) instalada e autenticada

## Uso

```bash
cd tools/ascension-ref

# Baixa manifesto + spells harvest (~1MB) e constrói o índice
python ascension_ref.py download

# Índice completo com exiles-db (~70MB: efeitos, escola, NPCs, talents)
python ascension_ref.py download --full

# Consulta
python ascension_ref.py lookup --spell 805844
python ascension_ref.py lookup --spell 100 --spell 101101 --format json
python ascension_ref.py lookup --name "soulrend"

# Ver o que já foi baixado
python ascension_ref.py status
```

## Estrutura

```
tools/ascension-ref/
├── ascension_ref.py  # a ferramenta (commitada)
├── README.md         # este arquivo (commitado)
├── data/             # IGNORADO: downloads (manifesto, jsons, .gz)
└── index/            # IGNORADO: spells.json (ID -> nome, fontes, tooltip)
```

## Fontes por spell

- **harvest**: `cachedata/lua/harvest/spells.json` — nome + tooltip
  observados no client (238 spells curados).
- **exiles**: mirror do site `db.exil.es` (`spells.jsonl.gz`, ~42MB,
  só com `--full`) — tabela de info, efeitos, ícone, escola, NPCs
  que conjuram. Requer `--full`.

## Fluxo com issues

```bash
# 1. pega o Spell ID da issue
python ../issue-sync/issue_sync.py sync
# 2. busca referência
python ascension_ref.py lookup --spell <ID>
# 3. implementa o SpellScript/AuraScript no core com os dados reais
```
