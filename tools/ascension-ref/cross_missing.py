"""Cruza talentos CoA ausentes no codigo com issues abertas."""
import glob
import gzip
import json
import os
import re
from collections import defaultdict

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

COMPOUND = ["knight-of-xoroth", "sun-cleric", "witch-doctor", "witch-hunter",
            "death-knight"]
tree_of = {}
missing = set()
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
            if x.get("type") == "spell" and int(x["key"]) >= 500000 \
                    and x["key"] not in ids:
                missing.add(x["key"])
                tree_of.setdefault(x["key"], set()).add(r["name"])


def norm(s):
    return re.sub(r"[^a-z0-9]", "", s.lower())


d = "tools/issue-sync/issues/open"
hits = defaultdict(list)
for fn in os.listdir(d):
    with open(os.path.join(d, fn), encoding="utf-8") as fh:
        content = fh.read()
    low = content.lower()
    num = re.search(r"^number: (\d+)", content, re.M).group(1)
    title = re.search(r"^title: \"(.*)\"", content, re.M).group(1)
    for sid in missing:
        name = idx.get(sid, {}).get("name", "")
        if not name:
            continue
        if sid in content or (len(norm(name)) >= 6 and norm(name) in low):
            hits[sid].append((num, title[:80]))

print("=== talentos ausentes COM issue aberta ===")
for sid in sorted(hits, key=int):
    name = idx.get(sid, {}).get("name", "?")
    trees = ", ".join(sorted(tree_of.get(sid, [])))
    print(f"{sid} {name} [{trees}]")
    for num, t in hits[sid]:
        print(f"   -> #{num}: {t}")

print()
print("=== talentos ausentes SEM issue ===")
for sid in sorted(missing - set(hits), key=int):
    name = idx.get(sid, {}).get("name", "?")
    trees = ", ".join(sorted(tree_of.get(sid, [])))
    print(f"  {sid}  {name}  [{trees}]")
