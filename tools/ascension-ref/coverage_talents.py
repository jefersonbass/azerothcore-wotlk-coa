"""Passada de cobertura exiles talents vs codigo (prototipo p/ comando coverage)."""
import glob
import gzip
import json
import re

# 1. Coletar IDs referenciados no modulo + SQLs
ids = set()
paths = (glob.glob("modules/mod-ascension-compat/src/*.cpp")
         + glob.glob("modules/mod-ascension-compat/src/*.h")
         + glob.glob("data/sql/updates/pending_db_world/*.sql")
         + glob.glob("modules/mod-ascension-compat/data/sql/db-world/*.sql"))
for p in paths:
    with open(p, encoding="utf-8", errors="replace") as f:
        txt = f.read()
    for m in re.finditer(r"\b(\d{6,7})\b", txt):
        ids.add(m.group(1))
print("IDs 6-7 digitos no modulo+SQL:", len(ids))

# 2. Indice de nomes
with open("tools/ascension-ref/index/spells.json", encoding="utf-8") as f:
    idx = json.load(f)

# 3. Spells de talento das classes custom (excluir reborn/none/general)
COMPOUND = ["knight-of-xoroth", "sun-cleric", "witch-doctor", "witch-hunter",
            "death-knight"]
custom_sp = set()
tree_of = {}
with gzip.open("tools/ascension-ref/data/exiles_talents.jsonl.gz",
               "rt", encoding="utf-8") as f:
    for line in f:
        line = line.strip()
        if not line:
            continue
        r = json.loads(line)
        k = r["key"].split("/")[0]
        cls = k
        for pre in COMPOUND:
            if k.startswith(pre):
                cls = pre
                break
        if k.startswith("reborn-") or k in ("none", "general-general1"):
            continue
        for x in r.get("structure", {}).get("references", []):
            if x.get("type") == "spell":
                custom_sp.add(x["key"])
                tree_of.setdefault(x["key"], set()).add(r["name"])

print("talent spells custom:", len(custom_sp))
covered = custom_sp & ids
missing = custom_sp - ids
print("cobertos (aparecem no modulo/sql):",
      len(covered), "=", round(100 * len(covered) / len(custom_sp), 1), "%")
print("ausentes:", len(missing), "=",
      round(100 * len(missing) / len(custom_sp), 1), "%")
print()
print("=== amostra dos ausentes (com nome e arvore) ===")
for sid in sorted(missing, key=int)[:25]:
    name = idx.get(sid, {}).get("name", "?")
    trees = ", ".join(sorted(tree_of.get(sid, [])))[:60]
    print(f"  {sid}  {name[:40]:<40} [{trees}]")
