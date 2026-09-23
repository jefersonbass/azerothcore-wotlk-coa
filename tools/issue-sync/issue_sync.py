#!/usr/bin/env python3
"""
issue-sync: exporta issues do GitHub para arquivos markdown locais.

Uso:
    python issue_sync.py extract [--repo OWNER/REPO] [--limit N]
    python issue_sync.py sync    [--repo OWNER/REPO] [--limit N]

- extract: baixa TODAS as issues (open + closed) e gera um .md por issue
           em issues/open/ ou issues/closed/.
- sync: verifica o que mudou no GitHub (novas, editadas, fechadas,
        reabertas) e atualiza/move apenas os arquivos afetados,
        para nao gerar duplicatas ao trabalhar em cima.

Requer: CLI `gh` instalada e autenticada. Apenas stdlib do Python.
"""

import argparse
import hashlib
import json
import os
import re
import subprocess
import sys
import time
from datetime import datetime, timezone

DEFAULT_REPO = "jealous-sound/azerothcore-wotlk-coa"

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
ISSUES_DIR = os.path.join(BASE_DIR, "issues")
OPEN_DIR = os.path.join(ISSUES_DIR, "open")
CLOSED_DIR = os.path.join(ISSUES_DIR, "closed")
STATE_FILE = os.path.join(BASE_DIR, "state.json")


class GhApiError(Exception):
    pass


def log(msg):
    print(msg, flush=True)


def die(msg):
    print(f"ERRO: {msg}", file=sys.stderr)
    sys.exit(1)


def gh_api(path, params=None, retries=5):
    """Chama `gh api` (GET) com retry em rate limit/falha transitoria."""
    if params:
        qs = "&".join(f"{k}={v}" for k, v in params.items())
        path = f"{path}?{qs}"
    cmd = ["gh", "api", path]
    for attempt in range(1, retries + 1):
        try:
            # Bytes + decode UTF-8 manual: text=True quebra no Windows
            # (cp1252) quando issues tem emoji/acentos.
            proc = subprocess.run(cmd, capture_output=True, check=True)
            stdout = proc.stdout.decode("utf-8", errors="replace")
        except FileNotFoundError:
            die("CLI `gh` nao encontrada no PATH. Instale: https://cli.github.com/")
        except subprocess.CalledProcessError as e:
            raw_err = e.stderr or b""
            err = raw_err.decode("utf-8", errors="replace").strip()
            if attempt < retries and (
                    "rate limit" in err.lower() or "secondary" in err.lower()
                    or "502" in err or "503" in err):
                wait = 10 * attempt
                log(f"  [retry {attempt}/{retries}] {err[:120]} — aguardando {wait}s...")
                time.sleep(wait)
                continue
            raise GhApiError(f"`gh api {path}` falhou: {err}")
        if not stdout.strip():
            if attempt < retries:
                wait = 10 * attempt
                log(f"  [retry {attempt}/{retries}] resposta vazia — aguardando {wait}s...")
                time.sleep(wait)
                continue
            raise GhApiError(f"`gh api {path}` retornou resposta vazia apos {retries} tentativas")
        return json.loads(stdout)


def is_issue(item):
    """A API de issues tambem retorna PRs; filtra para issues puras."""
    return "pull_request" not in item


def slugify(title, max_len=60):
    slug = re.sub(r"[^a-z0-9]+", "-", title.lower()).strip("-")
    return (slug[:max_len].rstrip("-") or "issue")


def filename_for(issue):
    return f"{issue['number']}-{slugify(issue.get('title') or '')}.md"


def target_dir(issue):
    return CLOSED_DIR if issue.get("state") == "closed" else OPEN_DIR


def render_markdown(issue, comments):
    """Monta o .md com frontmatter + corpo + comentarios (formato amigavel p/ IA)."""
    labels = [l["name"] for l in issue.get("labels", [])]
    assignees = [a["login"] for a in issue.get("assignees", [])]
    milestone = (issue.get("milestone") or {}).get("title", "")

    fm = [
        "---",
        f"number: {issue['number']}",
        f"title: {json.dumps(issue.get('title') or '')}",
        f"state: {issue.get('state')}",
        f"state_reason: {issue.get('state_reason') or ''}",
        f"author: {(issue.get('user') or {}).get('login', '')}",
        f"created_at: {issue.get('created_at')}",
        f"updated_at: {issue.get('updated_at')}",
        f"closed_at: {issue.get('closed_at') or ''}",
        f"labels: {json.dumps(labels)}",
        f"assignees: {json.dumps(assignees)}",
        f"milestone: {json.dumps(milestone)}",
        f"comments_count: {issue.get('comments', 0)}",
        f"url: {issue.get('html_url')}",
        "---",
    ]

    body = (issue.get("body") or "").strip() or "*_(sem descricao)_*"

    parts = [
        "\n".join(fm),
        "",
        f"# #{issue['number']} {issue.get('title') or ''}",
        "",
        body,
        "",
    ]

    if comments:
        parts += ["---", "", f"## Comentarios ({len(comments)})", ""]
        for c in comments:
            author = (c.get("user") or {}).get("login", "?")
            parts += [
                f"### @{author} em {c.get('created_at')}",
                "",
                (c.get("body") or "").strip() or "*_(vazio)_*",
                "",
            ]
    else:
        parts += ["---", "", "## Comentarios (0)", ""]

    parts += [f"_Exportado de {issue.get('html_url')} em "
              f"{datetime.now(timezone.utc).strftime('%Y-%m-%dT%H:%M:%SZ')}_", ""]
    return "\n".join(parts)


def content_hash(issue, comments):
    """Hash do conteudo relevante p/ detectar mudancas no sync."""
    cdata = [
        {"author": (c.get("user") or {}).get("login"),
         "created_at": c.get("created_at"),
         "updated_at": c.get("updated_at"),
         "body": c.get("body")}
        for c in comments
    ]
    blob = json.dumps({
        "title": issue.get("title"),
        "body": issue.get("body"),
        "state": issue.get("state"),
        "state_reason": issue.get("state_reason"),
        "labels": sorted(l["name"] for l in issue.get("labels", [])),
        "assignees": sorted(a["login"] for a in issue.get("assignees", [])),
        "milestone": (issue.get("milestone") or {}).get("title"),
        "closed_at": issue.get("closed_at"),
        "comments": cdata,
    }, sort_keys=True, ensure_ascii=False)
    return hashlib.sha256(blob.encode("utf-8")).hexdigest()


def fetch_all_issues(repo, state="all", limit=0):
    """Pagina todas as issues (metadados + body, sem comentarios)."""
    issues, page = [], 1
    while True:
        try:
            raw = gh_api(f"repos/{repo}/issues",
                         {"state": state, "per_page": 100, "page": page,
                          "direction": "asc"})
        except GhApiError as e:
            die(str(e))
        # O endpoint mistura issues e PRs: filtra, mas a paginacao segue
        # o tamanho BRUTO (uma pagina cheia de PRs nao significa fim).
        batch = [i for i in raw if is_issue(i)]
        issues.extend(batch)
        log(f"  pagina {page}: +{len(batch)} issues (total {len(issues)})")
        if limit and len(issues) >= limit:
            return issues[:limit]
        if len(raw) < 100:
            break
        page += 1
    return issues


def fetch_comments(repo, number):
    comments, page = [], 1
    while True:
        batch = gh_api(f"repos/{repo}/issues/{number}/comments",
                       {"per_page": 100, "page": page})
        if not batch:
            break
        comments.extend(batch)
        if len(batch) < 100:
            break
        page += 1
    return comments


def load_state():
    if os.path.exists(STATE_FILE):
        with open(STATE_FILE, encoding="utf-8") as f:
            return json.load(f)
    return {}


def save_state(state):
    with open(STATE_FILE, "w", encoding="utf-8") as f:
        json.dump(state, f, indent=2, ensure_ascii=False)


def remove_stale_copies(number, keep_path):
    keep = os.path.abspath(keep_path)
    for d in (OPEN_DIR, CLOSED_DIR):
        if not os.path.isdir(d):
            continue
        for fn in os.listdir(d):
            p = os.path.abspath(os.path.join(d, fn))
            if fn.startswith(f"{number}-") and p != keep:
                os.remove(p)


def write_issue(issue, comments, state):
    """Escreve (ou reescreve) o .md da issue; move entre pastas se mudou
    de estado ou de titulo. Retorna (acao, caminho)."""
    num = str(issue["number"])
    new_name = filename_for(issue)
    new_dir = target_dir(issue)
    new_path = os.path.join(new_dir, new_name)
    os.makedirs(new_dir, exist_ok=True)

    remove_stale_copies(num, new_path)

    old = state.get(num, {})

    with open(new_path, "w", encoding="utf-8") as f:
        f.write(render_markdown(issue, comments))

    state[num] = {
        "path": new_path,
        "state": issue.get("state"),
        "updated_at": issue.get("updated_at"),
        "comments": issue.get("comments", 0),
        "hash": content_hash(issue, comments),
    }

    if not old:
        return "nova", new_path
    if old.get("state") != issue.get("state"):
        return f"{old.get('state')}->{issue.get('state')}", new_path
    return "atualizada", new_path


def cmd_extract(args):
    log(f"[extract] {args.repo} -> {ISSUES_DIR}")
    os.makedirs(OPEN_DIR, exist_ok=True)
    os.makedirs(CLOSED_DIR, exist_ok=True)
    issues = fetch_all_issues(args.repo, limit=args.limit)
    log(f"[extract] {len(issues)} issues encontradas, baixando comentarios...")
    state = {}
    falhas = []
    for i, issue in enumerate(issues, 1):
        try:
            comments = fetch_comments(args.repo, issue["number"])
            write_issue(issue, comments, state)
        except GhApiError as e:
            falhas.append((str(issue["number"]), str(e)))
        if i % 50 == 0 or i == len(issues):
            log(f"  {i}/{len(issues)}...")
    save_state(state)
    if falhas:
        log(f"[extract] FALHAS: {len(falhas)} issues nao baixadas")
        for num, err in falhas[:20]:
            log(f"  #{num}: {err[:140]}")
    n_open = sum(1 for v in state.values() if v["state"] == "open")
    n_closed = len(state) - n_open
    log(f"[extract] OK: {len(state)} arquivos ({n_open} open, {n_closed} closed)")


def cmd_sync(args):
    log(f"[sync] {args.repo}")
    state = load_state()
    if not state:
        log("[sync] nenhum estado local; rode `extract` primeiro.")
        return
    issues = fetch_all_issues(args.repo, limit=args.limit)
    remote_nums = set()
    novas = atualizadas = movidas = 0
    falhas = []

    for issue in issues:
        num = str(issue["number"])
        remote_nums.add(num)
        old = state.get(num)

        precisa = (
            old is None
            or old.get("state") != issue.get("state")
            or old.get("updated_at") != issue.get("updated_at")
            or old.get("comments") != issue.get("comments", 0)
            or not (old.get("path") and os.path.exists(old["path"]))
        )
        if not precisa:
            continue
        try:
            comments = fetch_comments(args.repo, issue["number"])
            # Confirma pelo hash p/ nao reescrever sem mudanca real.
            if old and old.get("hash") == content_hash(issue, comments) \
                    and old.get("state") == issue.get("state") \
                    and old.get("path") and os.path.exists(old["path"]):
                state[num].update({"updated_at": issue.get("updated_at"),
                                   "comments": issue.get("comments", 0)})
                continue
            acao, path = write_issue(issue, comments, state)
        except GhApiError as e:
            falhas.append((num, str(e)))
            continue
        if acao == "nova":
            novas += 1
        elif "->" in acao:
            movidas += 1
            log(f"  #{num} {acao}: {os.path.basename(path)}")
        else:
            atualizadas += 1

    # Issues que sumiram do remoto (deletadas) -> remove arquivo local.
    removidas = 0
    for num in [n for n in state if n not in remote_nums]:
        p = state[num].get("path")
        if p and os.path.exists(p):
            os.remove(p)
        del state[num]
        removidas += 1
        log(f"  #{num} removida do remoto; arquivo local apagado")

    save_state(state)
    log(f"[sync] OK: {novas} novas, {atualizadas} atualizadas, "
        f"{movidas} movidas open<->closed, {removidas} removidas")
    if falhas:
        log(f"[sync] FALHAS: {len(falhas)} issues nao atualizadas nesta passada")
        for num, err in falhas[:20]:
            log(f"  #{num}: {err[:140]}")
    if args.limit:
        return
    verifica_remoto(args.repo)


def count_local(d):
    if not os.path.isdir(d):
        return 0
    return len([f for f in os.listdir(d) if f.endswith(".md")])


def fetch_remote_counts(repo):
    q = f"repo:{repo}+type:issue+state:"
    op = gh_api("search/issues", {"q": q + "open", "per_page": 1})["total_count"]
    cl = gh_api("search/issues", {"q": q + "closed", "per_page": 1})["total_count"]
    return op, cl


def verifica_remoto(repo):
    try:
        ro, rc = fetch_remote_counts(repo)
    except GhApiError as e:
        log(f"[sync] nao foi possivel conferir o remoto: {e}")
        return
    lo, lc = count_local(OPEN_DIR), count_local(CLOSED_DIR)
    log(f"[sync] conferencia: local {lo} open / {lc} closed | "
        f"remoto {ro} open / {rc} closed")
    if lo > ro:
        log(f"[sync] DIVERGENCIA: {lo - ro} arquivo(s) em open/ que o remoto ja fechou")
        sys.exit(3)
    if lc != rc:
        log(f"[sync] DIVERGENCIA: closed local {lc} != remoto {rc}")
        sys.exit(3)
    if ro > lo:
        log(f"[sync] {ro - lo} issue(s) novas no remoto desde esta passada")
    log("[sync] snapshot bate com o fork pai")


def main():
    ap = argparse.ArgumentParser(description="Exporta issues do GitHub p/ markdown local.")
    ap.add_argument("cmd", choices=["extract", "sync"])
    ap.add_argument("--repo", default=DEFAULT_REPO)
    ap.add_argument("--limit", type=int, default=0,
                    help="maximo de issues (0 = todas). util p/ teste.")
    args = ap.parse_args()
    if args.cmd == "extract":
        cmd_extract(args)
    else:
        cmd_sync(args)


if __name__ == "__main__":
    main()
