# CoA Issue Analyzer dashboard

Navigable analysis of all synced issues (tools/issue-sync/issues/) with charts,
filters and severity breakdown. Not committed to git (local tooling).

## Usage

```bat
:: 1. after every issue-sync, refresh the data:
python tools\issue-sync\analyze_issues.py

:: 2. open the dashboard (starts local server on :8765):
tools\issue-sync\open_dashboard.bat
```

Or manually: `python -m http.server 8765 --directory tools/issue-sync/dashboard`
then open http://localhost:8765/index.html

Note: opening index.html directly via file:// does NOT work (fetch of data.json
is blocked by the browser). Use the launcher or any static HTTP server.

## What it shows

- KPIs: total / open / closed / gamebreaking / high severity
- Doughnut: issues by heuristic category (click a slice to filter the table)
- Stacked bar: severity (high/mid/low) per category
- Horizontal bar: top 20 classes by issue count
- Bar: gamebreaking issues per category
- Table: full issue list with filters (state, category, severity, class, text search,
  gamebreaking-only chip), links to the upstream issue pages

## Classification heuristics (analyze_issues.py)

- Category: title-first keyword rules (spell/proc > talent > quest > npc > item >
  instance > ui > bots > infra); body only escalates to "gamebreaking" on crash
  evidence or when the title yields nothing.
- Severity: in-game report form severity (1/2/3) when present, else explicit
  "gamebreaking: yes" answer, else category-based default.
- Classes: matched against the CoA class roster in the title/body.
