# issue-sync

Exporta as issues do GitHub (`jealous-sound/azerothcore-wotlk-coa`) para
arquivos markdown locais, organizados por estado — para consulta e trabalho
com IA sem duplicatas.

**Tudo nesta pasta é local e NÃO é commitado** (ver `.gitignore` na raiz).

## Requisitos

- Python 3 (stdlib apenas, sem dependências)
- CLI [`gh`](https://cli.github.com/) instalada e autenticada (`gh auth login`)

## Uso

```bash
cd tools/issue-sync

# Baixa TODAS as issues (open + closed) — primeira vez
python issue_sync.py extract

# Depois: verifica o que mudou (novas, editadas, fechadas, reabertas)
# e atualiza/move apenas os arquivos afetados
python issue_sync.py sync

# Opções
python issue_sync.py extract --repo OUTRO/REPO   # outro repositório
python issue_sync.py extract --limit 10           # teste com poucas issues
```

## Estrutura

```
tools/issue-sync/
├── issue_sync.py   # a ferramenta (commitada)
├── README.md       # este arquivo (commitado)
├── issues/         # IGNORADO pelo git
│   ├── open/       # uma .md por issue aberta:  <numero>-<slug>.md
│   └── closed/     # idem, para issues fechadas
└── state.json      # IGNORADO pelo git (controle do sync)
```

## Formato do .md

Cada arquivo tem frontmatter YAML (número, título, estado, motivo do
fechamento, autor, labels, assignees, milestone, datas, contagem de
comentários, URL) + corpo da issue + todos os comentários. Pronto para
ser lido por IA.

## Como o sync evita duplicatas

- `state.json` guarda, por issue: caminho do arquivo, estado, `updated_at`,
  nº de comentários e hash do conteúdo.
- No `sync`, a ferramenta lista as issues (só metadados, rápido) e só
  rebaixa comentários/reexporta quem mudou de verdade (confirmação por hash).
- Issue fechada → `.md` movido de `open/` para `closed/` (e vice-versa se
  reaberta). Título editado → arquivo antigo apagado, novo criado.
- Issue deletada no GitHub → arquivo local apagado.
