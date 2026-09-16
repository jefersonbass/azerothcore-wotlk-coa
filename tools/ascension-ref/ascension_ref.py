#!/usr/bin/env python3
"""
ascension-ref: referencia local dos dados Ascension/CoA para corrigir issues.

Baixa arquivos pequenos e valiosos do repo hertigservices/ascension-data
(manifestos, spells harvest, catalogos exiles-db) e permite consulta por ID:

    python ascension_ref.py download [--repo OWNER/REPO]
    python ascension_ref.py lookup --spell 805844
    python ascension_ref.py lookup --item 12345 --quest 678
    python ascension_ref.py lookup --spell 805844 --format md
    python ascension_ref.py status

Requer: CLI `gh` instalada e autenticada. Apenas stdlib do Python.

Tudo em data/ e index/ e IGNORADO pelo git (ver .gitignore na raiz).
"""

import argparse
import base64
import gzip
import io
import json
import os
import subprocess
import sys
import tarfile
import time
import urllib.request

DEFAULT_REPO = "hertigservices/ascension-data"

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
DATA_DIR = os.path.join(BASE_DIR, "data")
INDEX_DIR = os.path.join(BASE_DIR, "index")
MANIFEST_FILE = os.path.join(DATA_DIR, "cache.json")

# Arquivos pequenos (<1MB) baixados direto do git (conteudo, nao packs).
GIT_FILES = {
    # manifesto principal -> tambem serve p/ downloads sob demanda via packs
    "manifests/cache-da4e17a09b90.json": "manifests_cache.json",
}

# Arquivos dentro dos packs (release) baixados sob demanda e cacheados.
# (nome no manifesto, tamanho aprox)
PACK_FILES = {
    "cachedata/lua/harvest/spells.json": "harvest_spells.json",
    "cachedata/lua/harvest/skillCards.json": "harvest_skillCards.json",
    "cachedata/lua/harvest/enums.json": "harvest_enums.json",
    "cachedata/lua/harvest/dungeons.json": "harvest_dungeons.json",
    "cachedata/lua/harvest/battlegrounds.json": "harvest_battlegrounds.json",
}

# Arquivos grandes do exiles-db (no git, download direto via raw).
# Baixados apenas com --full.
EXILES_BASE = ("supplemental/exiles-db/"
               "6bcecd0faa6c2e7084d8015e1d931431c30e3013a9122810d593868196a3578b")
EXILES_FILES = {
    f"{EXILES_BASE}/spells.jsonl.gz": "exiles_spells.jsonl.gz",      # ~42MB
    f"{EXILES_BASE}/talents.jsonl.gz": "exiles_talents.jsonl.gz",    # ~220KB
    f"{EXILES_BASE}/names.tsv.gz": "exiles_names.tsv.gz",            # ~5.7MB
    f"{EXILES_BASE}/manifest.json": "exiles_manifest.json",
}


def log(msg):
    print(msg, flush=True)


def die(msg):
    print(f"ERRO: {msg}", file=sys.stderr)
    sys.exit(1)


def run(cmd, retries=4):
    for attempt in range(1, retries + 1):
        try:
            proc = subprocess.run(cmd, capture_output=True, check=True)
            return proc.stdout.decode("utf-8", errors="replace")
        except FileNotFoundError:
            die("CLI `gh` nao encontrada no PATH. Instale: https://cli.github.com/")
        except subprocess.CalledProcessError as e:
            err = (e.stderr or b"").decode("utf-8", errors="replace").strip()
            if attempt < retries and ("rate limit" in err.lower()
                                      or "502" in err or "503" in err):
                wait = 10 * attempt
                log(f"  [retry {attempt}] aguardando {wait}s...")
                time.sleep(wait)
                continue
            die(f"falhou: {' '.join(cmd[:5])}... -> {err[:200]}")
    die("tentativas esgotadas")


def gh_content(repo, path):
    """Conteudo de arquivo pequeno via API (base64)."""
    out = run(["gh", "api", f"repos/{repo}/contents/{path}", "--jq", ".content"])
    return base64.b64decode("".join(out.split()))


def gh_download(repo, path, dest):
    """Download de arquivo grande via raw (stream)."""
    # pega o sha do arquivo p/ montar URL raw estavel
    meta = run(["gh", "api", f"repos/{repo}/contents/{path}", "--jq",
                "{sha: .sha, size: .size}"])
    info = json.loads(meta)
    url = (f"https://raw.githubusercontent.com/{repo}/"
           f"{default_branch(repo)}/{path}")
    log(f"  {path} ({info.get('size', '?')} bytes)")
    req = urllib.request.Request(url, headers={"User-Agent": "Mozilla/5.0"})
    os.makedirs(os.path.dirname(dest), exist_ok=True)
    with urllib.request.urlopen(req, timeout=300) as r, open(dest, "wb") as f:
        while True:
            chunk = r.read(1024 * 1024)
            if not chunk:
                break
            f.write(chunk)


_branch_cache = {}


def default_branch(repo):
    if repo not in _branch_cache:
        out = run(["gh", "api", f"repos/{repo}", "--jq", ".default_branch"])
        _branch_cache[repo] = out.strip()
    return _branch_cache[repo]


def load_cache_manifest():
    if not os.path.exists(MANIFEST_FILE):
        die("manifesto cache.json nao encontrado. Rode `download` primeiro.")
    with open(MANIFEST_FILE, encoding="utf-8") as f:
        return json.load(f)


def fetch_pack_member(manifest, target):
    """Baixa o pack que contem `target` e extrai o membro (cacheado em data/)."""
    files, packs = manifest["files"], manifest["packs"]
    if target not in files:
        die(f"{target} nao existe no manifesto")
    entry = files[target]
    member_hash = entry["parts"][0]["sha256"]
    pack_name = entry["parts"][0]["pack"]
    if len(entry["parts"]) > 1:
        die(f"{target} tem multiplas partes; download manual necessario")
    url = packs[pack_name]["url"]
    log(f"  pack {pack_name} ({packs[pack_name]['bytes']} bytes)...")
    req = urllib.request.Request(url, headers={"User-Agent": "Mozilla/5.0"})
    with urllib.request.urlopen(req, timeout=600) as r:
        data = r.read()
    tf = tarfile.open(fileobj=io.BytesIO(data))
    member = tf.extractfile(member_hash)
    if member is None:
        die(f"membro {member_hash[:12]}... nao achado no pack")
    return member.read()


def ensure_local(name, manifest=None):
    """Garante arquivo em data/ (baixa do pack se preciso). Retorna caminho."""
    dest = os.path.join(DATA_DIR, name)
    if os.path.exists(dest):
        return dest
    # acha o target correspondente
    for target, local in PACK_FILES.items():
        if local == name:
            if manifest is None:
                manifest = load_cache_manifest()
            log(f"baixando {target}...")
            content = fetch_pack_member(manifest, target)
            os.makedirs(DATA_DIR, exist_ok=True)
            with open(dest, "wb") as f:
                f.write(content)
            return dest
    die(f"arquivo desconhecido: {name}")


# ---------------------------------------------------------------- index

def build_spell_index():
    """Indice ID spell -> {nome, fontes} a partir de harvest + exiles."""
    idx = {}

    # 1. harvest spells.json (tooltip/nome observados no client)
    hp = ensure_local("harvest_spells.json")
    with open(hp, encoding="utf-8") as f:
        harvest = json.load(f)
    for sid, sp in harvest.items():
        idx[str(sid)] = {"name": sp.get("name", ""),
                         "sources": ["harvest"],
                         "tooltip": sp.get("tooltip", [])}

    # 2. exiles spells.jsonl.gz (site db.exil.es: efeitos, escola, NPCs)
    ep = os.path.join(DATA_DIR, "exiles_spells.jsonl.gz")
    n_ex = 0
    if os.path.exists(ep):
        with gzip.open(ep, "rt", encoding="utf-8", errors="replace") as f:
            for line in f:
                line = line.strip()
                if not line:
                    continue
                try:
                    rec = json.loads(line)
                except json.JSONDecodeError:
                    continue
                sid = str(rec.get("id") or rec.get("spell_id") or "")
                if not sid:
                    continue
                e = idx.setdefault(sid, {"name": rec.get("name", ""),
                                         "sources": [], "tooltip": []})
                if not e["name"] and rec.get("name"):
                    e["name"] = rec["name"]
                if "exiles" not in e["sources"]:
                    e["sources"].append("exiles")
                e["exiles"] = {k: rec.get(k) for k in
                               ("url", "icon", "school", "effects",
                                "casting_npcs", "info") if rec.get(k) is not None}
                n_ex += 1
    log(f"indice spells: {len(idx)} ids ({n_ex} com dados exiles)")

    os.makedirs(INDEX_DIR, exist_ok=True)
    with open(os.path.join(INDEX_DIR, "spells.json"), "w", encoding="utf-8") as f:
        json.dump(idx, f, ensure_ascii=False)
    return idx


def load_spell_index():
    p = os.path.join(INDEX_DIR, "spells.json")
    if not os.path.exists(p):
        return build_spell_index()
    with open(p, encoding="utf-8") as f:
        return json.load(f)


def search_names(query, limit=20):
    """Busca por nome no indice + names.tsv do exiles (se baixado)."""
    q = query.lower()
    idx = load_spell_index()
    out = []
    for sid, sp in idx.items():
        if q in (sp.get("name") or "").lower():
            out.append((sid, sp.get("name"), ",".join(sp.get("sources", []))))
        if len(out) >= limit:
            break
    return out


# ---------------------------------------------------------------- cmds

def cmd_download(args):
    os.makedirs(DATA_DIR, exist_ok=True)
    log(f"[download] repo {args.repo}")

    # 1. manifesto cache.json (pequeno, via API)
    log("manifesto cache.json...")
    raw = gh_content(args.repo, "datasets/cache.json")
    with open(MANIFEST_FILE, "wb") as f:
        f.write(raw)
    manifest = json.loads(raw.decode("utf-8"))
    log(f"  {len(manifest['files'])} arquivos, {len(manifest['packs'])} packs")

    # 2. arquivos pequenos dos packs (spells harvest etc.)
    for target, local in PACK_FILES.items():
        dest = os.path.join(DATA_DIR, local)
        if os.path.exists(dest) and not args.force:
            log(f"  {local} (ja existe)")
            continue
        log(f"  {target}...")
        content = fetch_pack_member(manifest, target)
        with open(dest, "wb") as f:
            f.write(content)

    # 3. exiles-db (grandes, so com --full)
    if args.full:
        for path, local in EXILES_FILES.items():
            dest = os.path.join(DATA_DIR, local)
            if os.path.exists(dest) and not args.force:
                log(f"  {local} (ja existe)")
                continue
            gh_download(args.repo, path, dest)
    else:
        log("  (exiles-db pulado; use --full para spells/talents/names completos)")

    # 4. indice
    log("construindo indice...")
    build_spell_index()
    log("[download] OK")


def fmt_spell(sid, sp, fmt):
    if fmt == "json":
        return json.dumps({"id": sid, **sp}, indent=1, ensure_ascii=False)
    lines = [f"# Spell {sid}: {sp.get('name', '?')}",
             f"fontes: {', '.join(sp.get('sources', []))}", ""]
    if sp.get("tooltip"):
        lines += ["## Tooltip (client)", ""]
        lines += [f"- {t}" for t in sp["tooltip"] if t.strip()]
        lines += [""]
    if sp.get("exiles"):
        lines += ["## Exiles DB", "",
                  json.dumps(sp["exiles"], indent=1, ensure_ascii=False)[:3000],
                  ""]
    return "\n".join(lines)


def cmd_lookup(args):
    idx = load_spell_index()
    found_any = False

    for sid in args.spell or []:
        sp = idx.get(str(sid))
        if sp:
            print(fmt_spell(sid, sp, args.format))
            found_any = True
        else:
            print(f"Spell {sid}: NAO ENCONTRADO no indice local.")
            print("(rode `download --full` p/ indice completo, ou verifique o ID)")
        print()

    if args.name:
        res = search_names(args.name)
        if res:
            print(f"## Busca '{args.name}' ({len(res)} resultados)")
            for sid, name, src in res:
                print(f"  {sid}  {name}  [{src}]")
            found_any = True
        else:
            print(f"Nada encontrado para '{args.name}'.")

    if not found_any and not (args.spell or args.name):
        die("informe --spell ID ou --name TEXTO")


def cmd_status(args):
    print(f"repo: {args.repo}")
    print(f"data/: {'ausente' if not os.path.isdir(DATA_DIR) else ''}")
    if os.path.isdir(DATA_DIR):
        for n in sorted(os.listdir(DATA_DIR)):
            p = os.path.join(DATA_DIR, n)
            print(f"  {n}  {os.path.getsize(p)} bytes")
    ip = os.path.join(INDEX_DIR, "spells.json")
    if os.path.exists(ip):
        with open(ip, encoding="utf-8") as f:
            idx = json.load(f)
        print(f"indice spells: {len(idx)} ids")
    else:
        print("indice spells: nao construido")


def main():
    ap = argparse.ArgumentParser(description="Referencia local Ascension/CoA.")
    ap.add_argument("--repo", default=DEFAULT_REPO)
    sub = ap.add_subparsers(dest="cmd", required=True)

    d = sub.add_parser("download", help="baixa dados e constroi indice")
    d.add_argument("--full", action="store_true",
                   help="inclui exiles-db (~70MB)")
    d.add_argument("--force", action="store_true",
                   help="rebaixa mesmo se existir")

    l = sub.add_parser("lookup", help="consulta spell por ID ou nome")
    l.add_argument("--spell", action="append",
                   help="ID do spell (pode repetir)")
    l.add_argument("--name",
                   help="busca por nome (substring)")
    l.add_argument("--format", choices=["md", "json"], default="md")

    sub.add_parser("status", help="mostra o que ja foi baixado")

    args = ap.parse_args()
    if args.cmd == "download":
        cmd_download(args)
    elif args.cmd == "lookup":
        cmd_lookup(args)
    elif args.cmd == "status":
        cmd_status(args)


if __name__ == "__main__":
    main()
