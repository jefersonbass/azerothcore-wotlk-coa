-- Glyphs of Power (520145, issue 498): the Runemaster passive authors aura 42 (proc trigger
-- spell) with TriggerSpell 706528, but Spell.dbc gives the record ProcFlags 0, so
-- SpellMgr::LoadSpellProcs never generated a proc entry and the aura was never prepared
-- (Aura::GetProcEffectMask returns 0 without one) — same defect as Fists of Power (805796).
-- The payload 706528 is effect 137 (ENERGIZE_PCT, BasePoints 2) targeted at the caster, so a
-- proc restores 2% of maximum mana; the tooltip's "3%" does not match the authored value.
-- ProcFlags 69652 is the damage-dealt set used by the class proc rows: done melee auto attack,
-- done melee-class spell and done negative spell of the none/magic damage classes, so any
-- direct damage school qualifies. SpellTypeMask 1 (damage) and SpellPhaseMask 2 (on hit)
-- mirror those rows. Chance 0 defers to the record's own ProcChance (10) — the tooltip's 10%.
DELETE FROM `spell_proc` WHERE `SpellId` = 520145;
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(520145, 0, 0, 0, 0, 0, 69652, 1, 2, 0, 0, 0, 0, 0, 0, 0);
