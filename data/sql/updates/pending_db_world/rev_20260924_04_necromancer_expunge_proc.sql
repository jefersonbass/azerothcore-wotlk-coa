-- #4058 Expunge (705754): "Casting Command spells on an enemy now applies Expunge if they are Diseased."
--   The record ships ProcFlags 0 in the DBC, so SpellMgr::LoadSpellProcs skips it ("Skip if no proc
--   flags in DBC", src/server/game/Spells/SpellMgr.cpp) and no fallback entry is generated;
--   Aura::GetProcEffectMask then returns 0 for any aura with no proc entry ("only auras with spell
--   proc entry can trigger proc", src/server/game/Spells/Auras/SpellAuras.cpp), so the aura never
--   fires and Casting a Command spell on a diseased enemy applies nothing.
--
--   Same defect and same shape as rev_20260922_94_necromancer_dead_procs.sql, which recorded this
--   exact clause for Forbidden Technique (561215): the clause names every Command spell, and
--   AscensionNecromancer.h Command() is Family(info, 2, 16777216), so SpellFamilyName 29 with
--   SpellFamilyMask2 16777216 matches exactly that set. Cast clause, so SpellPhaseMask 1
--   (PROC_SPELL_PHASE_CAST) with ProcFlags 87312, the shape recorded there. The record's own
--   ProcChance stays in charge (the columns left at 0 do not override it).
--
--   Two same-named variants exist: the aura's authored EffectTriggerSpell is 500443 ("Blast a
--   diseased enemy for ... Plague Damage"), while the talent's own tooltip carries the
--   @s:500343:0@ directive for the other one, 500343 ("Increases the target's damage taken from
--   your Necromancer diseases ..."). This row only makes the authored trigger reachable; which of
--   the two the talent is meant to deliver is a separate data-side question and is not changed here.
DELETE FROM `spell_proc` WHERE `SpellId` = 705754;
INSERT INTO `spell_proc`
(`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`,
 `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`,
 `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(705754, 0, 29, 0, 0, 16777216, 87312, 7, 1, 0, 0, 0, 0, 0, 0, 0);
