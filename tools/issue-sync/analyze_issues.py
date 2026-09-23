"""Analyze synced GitHub issues and emit JSON for the dashboard.

Usage:  python analyze_issues.py
Input:  issues/open/*.md, issues/closed/*.md  (written by issue_sync.py)
Output: dashboard/data.json  (then open dashboard/index.html in a browser)

Classification is heuristic (title + body keywords + structured fields from the
in-game report form: Category/Severity/Gamebreaking + Spell/Quest/Item/NPC header).
Re-run after every issue-sync to refresh the dashboard.
"""
from __future__ import annotations

import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parent
OUT = ROOT / "dashboard"

# Ordered: first match wins. Title is checked first (most reliable signal);
# body is only used for categories that need narrative context (crash etc).
CATEGORY_RULES = [
    ("gamebreaking", r"game.?break|server crash|world server|core dump|stuck in combat|cannot (log|login|log in)|kicked|disconnect(ed)? (from|at)|character (corrupt|lost|deleted)|progress (lost|wiped)|crash(es|ed|ing)?\b"),
    ("class-spell", r"\b(spell|ability|proc|aura|buff|debuff|dot|hot\b|cooldown|cd\b|cast|channel|immune|resist|not implemented|script|handler|trigger)\b"),
    ("class-talent", r"\b(talent|talents|spec|specialization|skill book|mastery|passive)\b"),
    ("class-mechanic", r"\b(combo point|eclipse|stance|form|shapeshift|resource|energy|rage|mana|rune|soul shard|charge|stack)\b"),
    ("quest", r"\b(quest|objective|turn.?in)\b"),
    ("npc-creature", r"\b(npc|creature|mob|spawn|pathing|patrol|body pull)\b"),
    ("item-loot", r"\b(item|loot|drop rate|vendor|buy|sell|equip|weapon|armor|trinket|transmog)\b"),
    ("instance-dungeon", r"\b(dungeon|raid|instance|boss|trash|reset|lockout)\b"),
    ("client-ui", r"\b(client|ctd|ui\b|interface|addon|frame|tooltip|model|texture|animation|sound)\b"),
    ("bots-pets", r"\b(bot|playerbot|pet|minion|totem|companion)\b"),
    ("server-infra", r"\b(server|realm|queue|lag|latency|desync|rub.?band|database|anti.?cheat|gm command)\b"),
    ("other", r".*"),
]

# Body-only categories (narrative context needed, title never says these).
BODY_ONLY_RULES = [
    ("gamebreaking", r"crash(es|ed|ing)?\b|server crash|core dump|disconnect(ed)? (from|at)|character (corrupt|lost|deleted)"),
]

CLASS_NAMES = [
    "Barbarian", "Witch Doctor", "Felsworn", "Runemaster", "Sun Cleric", "Cultist",
    "Bloodmage", "Pyromancer", "Witch Hunter", "Knight of Xoroth", "Chronomancer",
    "Reaper", "Primalist", "Starcaller", "Stormbringer", "Guardian", "Templar",
    "Necromancer", "Venomancer", "Tinker", "Ranger",
    # vanilla-style classes (only counted when the report is about them explicitly)
    "Death Knight", "Rogue", "Mage", "Priest", "Warlock", "Warrior", "Paladin",
    "Hunter", "Shaman", "Druid",
]

# Generic words that would false-positive vanilla class names (e.g. "mage" in "imagem").
CLASS_WORD_BOUNDARY = {"Mage": r"\bmage\b", "Priest": r"\bpriest\b", "Rogue": r"\brogue\b",
                       "Hunter": r"(?<!\bwitch )(?<!\bdemon )(?<!\bshadow )\bhunter\b"}


def classify(text: str) -> str:
    low = text.lower()
    for name, pattern in CATEGORY_RULES:
        if re.search(pattern, low):
            return name
    return "other"


def extract_classes(text: str) -> list[str]:
    found = set()
    for cls in CLASS_NAMES:
        pat = CLASS_WORD_BOUNDARY.get(cls, re.escape(cls))
        if re.search(pat, text, re.I):
            found.add(cls)
    return sorted(found)


def parse_issue(path: Path, state: str) -> dict:
    txt = path.read_text(encoding="utf-8", errors="replace")
    m = re.search(r"^number:\s*(\d+)", txt, re.M)
    num = int(m.group(1)) if m else 0
    m = re.search(r'^title:\s*"?(.*?)"?\s*$', txt, re.M)
    title = (m.group(1) if m else path.stem).strip(' "')

    m = re.search(r"^Category:\s*(\d+)", txt, re.M)
    form_cat = int(m.group(1)) if m else None
    m = re.search(r"^Severity:\s*(\d+)", txt, re.M)
    form_sev = int(m.group(1)) if m else None
    m = re.search(r"#### Is this a Gamebreaking Issue\s*\n+(.+?)\n", txt)
    gb_raw = m.group(1).strip().lower() if m else ""

    m = re.search(r"^#### (Spell|Quest|Item|NPC|Talent|Creature|GameObject)\b", txt, re.M)
    subject = m.group(1) if m else ""

    body = txt
    # classify by title first (x3 weight equivalent: run rules on title alone);
    # if title yields only "other", fall back to full-text rules
    category = classify(title)
    if category == "other":
        category = classify(title + "\n" + body)
    else:
        # body can still escalate to gamebreaking if it describes crashes
        for name, pattern in BODY_ONLY_RULES:
            if re.search(pattern, body, re.I) and name == "gamebreaking":
                # only escalate when title didn't clearly indicate a benign category
                if category not in ("quest", "npc-creature", "item-loot", "client-ui"):
                    category = "gamebreaking"
                break

    # severity: explicit form severity > gamebreaking flag > heuristic category
    if form_sev is not None and form_sev > 0:
        severity = {1: "low", 2: "mid", 3: "high"}.get(form_sev, "mid")
    elif gb_raw in ("yes", "yes for this spec") or gb_raw.startswith("y"):
        severity = "high"
    elif category == "gamebreaking":
        severity = "high"
    elif category in ("class-spell", "class-talent", "class-mechanic", "quest"):
        severity = "mid"
    else:
        severity = "low"

    # gamebreaking explicit flag
    gamebreaking = gb_raw.startswith(("yes", "yep", "yee")) or category == "gamebreaking"

    # subject-specific id if present (Spell [ID: 123] / Quest ID: 123)
    ids = re.findall(r"ID:\s*(\d+)", txt)[:3]

    return {
        "n": num,
        "state": state,
        "title": title,
        "category": category,
        "severity": severity,
        "gamebreaking": gamebreaking,
        "formCategory": form_cat,
        "formSeverity": form_sev,
        "subject": subject,
        "ids": ids,
        "classes": extract_classes(txt),
    }


def main() -> None:
    issues = []
    for state in ("open", "closed"):
        d = ROOT / "issues" / state
        if not d.is_dir():
            continue
        for path in sorted(d.glob("*.md")):
            try:
                issues.append(parse_issue(path, state))
            except Exception as e:  # noqa: BLE001 - keep going, count broken files
                print(f"skip {path.name}: {e}")

    OUT.mkdir(exist_ok=True)
    (OUT / "data.json").write_text(
        json.dumps({"generated": __import__("datetime").datetime.now().isoformat(timespec="seconds"),
                    "issues": issues}, ensure_ascii=False),
        encoding="utf-8",
    )
    print(f"OK: {len(issues)} issues -> dashboard/data.json")


if __name__ == "__main__":
    main()
