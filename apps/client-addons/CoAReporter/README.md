# CoAReporter (client addon, WoW 3.3.5 / Ascension CoA)

In-game bug reporter wired to the server's `COABUG` protocol
(`modules/mod-ascension-compat/src/CoABugReportService.h`).

## Install

Copy this folder to the client while it is closed:

```
E:\Jogos\ascension-live\Interface\AddOns\CoAReporter
```

Enable "Load out of date AddOns" if the client asks (TOC targets 3.3.5 build 12340).

## Use

- `/bug` or `/report` opens the form.
- Pick a type: **Quest / NPC / Item / Spell / Talent / Other**.
- IDs auto-fill:
  - Quest: first quest-log entry pre-fills (ID + title).
  - NPC: target an NPC first — name pre-fills, ID parsed from the target GUID on submit.
  - Item: shift-click a bag item into the picker (link carries the item ID).
  - Spell: shift-click a spellbook spell into the picker (link carries the spell ID).
  - Talent: open Talents, then `/run CoAReporterUI_CaptureTalent(tab, index)`.
  - Other: position only (the server always adds class/level/map/position/revision).
- Fill title + observed/expected behavior, press **Send report**.
- **Check status** queries the server for the queued issue number.

## Server side

Requires `CoABugReport.Enable = 1` + `CoABugReport.SpoolDirectory` in
`env/dist/etc/modules/coa_bugreport.conf` (see `conf/coa_bugreport.conf.dist`).
Reports land as `<account>-<request>.report` files; the optional relay
(`modules/mod-ascension-compat/apps/bugreport/relay.py`) publishes them to GitHub.
