-- Vile (705083), issue #2515: "Removes the Focus cost of Sapsting and causes it to increase the duration
-- of your Deepwood Poison."
--
-- Measured before writing:
--   * 705083 carries a live SPELL_AURA_PROC_TRIGGER_SPELL (aura 42) on 706507 Vile. Spell.dbc gives the
--     record ProcFlags 0 and the world DB holds no `spell_proc` row, so SpellMgr::LoadSpellProcs skips it
--     ("Skip if no proc flags in DBC") and Aura::GetProcEffectMask returns 0 before any effect is
--     examined - the proc has never fired.
--   * ProcChance is 100, so the aura is a marker whose event is casting Sapsting, and Chance stays 0 so
--     the loader keeps using it (SpellMgr.cpp:2122).
--   * 706507 is a single SPELL_EFFECT 177 with base points 1999 on the caster. Effect 177 is implemented
--     in the core and is not in the EffectNULL set, so the payload delivers.
--   * Sapsting exists once, 560018, with SpellFamilyName 27 and SpellFamilyFlags [0, 0, 0] - no bit at
--     all, so the mask cannot isolate it and the row drops the mask (family 0) and lets
--     spell_ascension_spell_list_talent_proc filter by the rule's spell list, which carries 560018. Same
--     case as Hailfire and the convention recorded at rev_20260921_40_ranger_dead_procs.sql:1354.
--   * ProcFlags 69904 = the four damage-by-ability classes with no auto-attack bits, SpellTypeMask 1
--     (DAMAGE) and SpellPhaseMask 2 (HIT).
--
-- The Rules entry is added in AscensionSpellListTalentProcs.h (17 -> 18).
DELETE FROM `spell_proc` WHERE `SpellId` = 705083;
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(705083, 0, 0, 0, 0, 0, 69904, 1, 2, 0, 0, 0, 0, 0, 0, 0);

DELETE FROM `spell_script_names` WHERE `spell_id` = 705083 AND `ScriptName` = 'spell_ascension_spell_list_talent_proc';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(705083, 'spell_ascension_spell_list_talent_proc');
