"""Gera o roteiro de teste de talentos (talent_test_roadmap.md) com os nomes
reais das classes no CoA: classes custom (12-32) e classes nativas = Reborn.

Saida: tools/ascension-ref/data/talent_test_roadmap.md (data/ ignorado).
"""
import glob
import gzip
import json
import os
import re
from collections import defaultdict

# ---- ids referenciados no codigo + SQL ----
ids = set()
paths = (glob.glob("modules/mod-ascension-compat/src/*.cpp")
         + glob.glob("modules/mod-ascension-compat/src/*.h")
         + glob.glob("data/sql/updates/pending_db_world/*.sql")
         + glob.glob("modules/mod-ascension-compat/data/sql/db-world/*.sql"))
for p in paths:
    with open(p, encoding="utf-8", errors="replace") as f:
        for m in re.finditer(r"\b(\d{6,7})\b", f.read()):
            ids.add(m.group(1))

with open("tools/ascension-ref/index/spells.json", encoding="utf-8") as f:
    idx = json.load(f)

# ---- talentos CoA (exiles) ----
COMPOUND = ["knight-of-xoroth", "sun-cleric", "witch-doctor", "witch-hunter",
            "death-knight"]
spells = set()
tree_of = {}
# Classes do client CoA (coluna client_name de ascension_custom_class).
# Arvores nativas (Warrior/Druid/Mage/...) e reborn-* NAO sao jogaveis
# no client CoA e ficam fora do roteiro.
CLIENT_CLASSES = {
    "Barbarian", "Witch Doctor", "Felsworn", "Witch Hunter",
    "Stormbringer", "Knight of Xoroth", "Guardian", "Templar",
    "Bloodmage", "Ranger", "Chronomancer", "Necromancer", "Pyromancer",
    "Cultist", "Starcaller", "Sun Cleric", "Tinker", "Venomancer",
    "Reaper", "Primalist", "Runemaster",
}
with gzip.open("tools/ascension-ref/data/exiles_talents.jsonl.gz",
               "rt", encoding="utf-8") as f:
    for line in f:
        line = line.strip()
        if not line:
            continue
        r = json.loads(line)
        k = r["key"].split("/")[0]
        if k.startswith("reborn-") or k in ("none", "general-general1"):
            continue
        for x in r.get("structure", {}).get("references", []):
            if x.get("type") == "spell" and int(x["key"]) >= 500000:
                spells.add(x["key"])
                tree_of.setdefault(x["key"], set()).add(r["name"])


def cls_of(tree_name):
    """Nome da classe client a partir do nome da arvore.

    Retorna vazio para arvores que nao pertencem a uma classe client
    (nativas: 'Warrior — Fury', 'Mage — Arcane', etc.).
    """
    base = tree_name.split("\u2014")[0].strip()
    if base in CLIENT_CLASSES:
        return base
    return ""
    return base


def norm(s):
    return re.sub(r"[^a-z0-9]", "", s.lower())


# ---- issues abertas: autor + spells citados ----
d = "tools/issue-sync/issues/open"
issue_by_spell = defaultdict(list)
for fn in os.listdir(d):
    with open(os.path.join(d, fn), encoding="utf-8") as fh:
        content = fh.read()
    low = content.lower()
    num = re.search(r"^number: (\d+)", content, re.M).group(1)
    author = re.search(r"^author: (.*)$", content, re.M).group(1).strip()
    title = re.search(r"^title: \"(.*)\"", content, re.M).group(1)
    human = author != "Corfirean"
    for sid in spells:
        name = idx.get(sid, {}).get("name", "")
        if not name:
            continue
        if sid in content or (len(norm(name)) >= 6 and norm(name) in low):
            issue_by_spell[sid].append((num, author, title, human))

missing = spells - ids

# ---- classes CoA custom (nomes oficiais do cliente) ----
OFFICIAL = {
    "Barbarian": "Barbarian", "Witch Doctor": "Witch Doctor",
    "Felsworn": "Felsworn", "Witch Hunter": "Witch Hunter",
    "Stormbringer": "Stormbringer", "Knight of Xoroth": "Knight of Xoroth",
    "Guardian": "Guardian", "Templar": "Templar",
    "Bloodmage": "Bloodmage", "Ranger": "Ranger",
    "Chronomancer": "Chronomancer", "Necromancer": "Necromancer",
    "Pyromancer": "Pyromancer", "Cultist": "Cultist",
    "Starcaller": "Starcaller", "Sun Cleric": "Sun Cleric",
    "Tinker": "Tinker", "Venomancer": "Venomancer", "Reaper": "Reaper",
    "Primalist": "Primalist", "Runemaster": "Runemaster",
}

classes = defaultdict(lambda: {"ausente": [], "bugado_humano": [],
                               "bugado_corf": 0})
for sid in sorted(missing, key=int):
    name = idx.get(sid, {}).get("name", "?")
    iss = "; ".join(f"#{n}" for n, a, t, h in issue_by_spell.get(sid, []))
    for t in tree_of[sid]:
        cls = cls_of(t)
        if cls:
            classes[cls]["ausente"].append((sid, name, t, iss))
for sid in sorted(spells & ids, key=int):
    if sid in issue_by_spell:
        name = idx.get(sid, {}).get("name", "?")
        for n, a, t, h in issue_by_spell[sid]:
            for tr in tree_of[sid]:
                cls = cls_of(tr)
                if not cls:
                    continue
                if h:
                    classes[cls]["bugado_humano"].append(
                        (sid, name, n, a, t))
                else:
                    classes[cls]["bugado_corf"] += 1


def label(cls):
    """Nome oficial do client CoA."""
    return OFFICIAL.get(cls, cls)


out = ["# Roteiro de teste de talentos — CoA (por classe)",
       "",
       "Gerado por `class_test_list.py` (exiles talents vs codigo vs issues",
       "abertas). Testar in-game antes de abrir issue ou implementar.",
       "",
       "Contem somente as 21 classes jogaveis do client CoA",
       "(ascension_custom_class.client_name). Arvores de classes nativas",
       "(Warrior, Mage, etc.) do mirror exiles ficam fora do escopo.",
       "",
       "Legenda:",
       "- **AUSENTE** — o ID do talento nao aparece em nenhum .cpp/.h/SQL do",
       "  modulo; provavelmente nao faz nada. Confirmar no jogo e implementar.",
       "- **TESTAR** — o talento TEM codigo, mas issue humana o cita como",
       "  bugado; reproduzir o sintoma reportado e confirmar/descartar.",
       ""]

order = sorted(classes, key=lambda c: (-len(classes[c]["ausente"]),
                                       -len(classes[c]["bugado_humano"]),
                                       label(c)))
total_a = total_t = 0
for cls in order:
    c = classes[cls]
    if not (c["ausente"] or c["bugado_humano"] or c["bugado_corf"]):
        continue
    out.append(f"## {label(cls)}")
    out.append("")
    if c["ausente"]:
        total_a += len(c["ausente"])
        out.append(f"### AUSENTE no codigo ({len(c['ausente'])})")
        out.append("")
        out.append("| Spell ID | Talento | Arvore | Issues |")
        out.append("|---|---|---|---|")
        for sid, name, tree, iss in sorted(c["ausente"]):
            tag = iss or "—"
            out.append(f"| {sid} | {name} | {tree} | {tag} |")
        out.append("")
        out.append("Teste sugerido: aprender o talento, acionar a condicao dele")
        out.append("(dano recebido, cast, kill, etc.) e observar se o efeito")
        out.append("aplica. Se nada acontecer, confirmado ausente.")
        out.append("")
    if c["bugado_humano"]:
        out.append(f"### TESTAR — citado em issue humana")
        out.append("")
        out.append("| Spell ID | Talento | Issue | Sintoma reportado |")
        out.append("|---|---|---|---|")
        seen = set()
        for sid, name, num, author, title in sorted(c["bugado_humano"]):
            if (sid, num) in seen:
                continue
            seen.add((sid, num))
            total_t += 1
            t = title if len(title) <= 70 else title[:67] + "..."
            out.append(f"| {sid} | {name} | #{num} ({author}) | {t} |")
        out.append("")
        out.append("Teste sugerido: reproduzir o cenario da issue (nivel, spec,")
        out.append("condicao) e verificar se o sintoma ainda ocorre no build")
        out.append("atual. Se nao ocorrer, fechar a issue com o resultado.")
        out.append("")
    if c["bugado_corf"]:
        out.append(f"_+ {c['bugado_corf']} citacoes em issues auto-geradas")
        out.append("(Corfirean) fora do escopo._")
        out.append("")

out.insert(10, f"**Resumo: {total_a} talentos ausentes, "
              f"{total_t} citacoes humanas para testar.**")
out.insert(11, "")

dest = os.path.join("tools", "ascension-ref", "data", "talent_test_roadmap.md")
with open(dest, "w", encoding="utf-8") as f:
    f.write("\n".join(out))
print(f"roteiro: {dest}")
print(f"ausentes: {total_a} | citacoes humanas: {total_t}")
