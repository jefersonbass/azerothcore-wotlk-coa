-- Excruciating Poison (704544), issue #2081: "Assault now reduces healing done to the target by
-- $800090s1% for $800090d, stacking $800090u times."
--
-- Measured before writing:
--   * 704544 carries a live SPELL_AURA_PROC_TRIGGER_SPELL (aura 42) on 800090 Excruciating Poison.
--     Spell.dbc gives the record ProcFlags 0 and the world DB holds no `spell_proc` row, so
--     SpellMgr::LoadSpellProcs skips it ("Skip if no proc flags in DBC") and Aura::GetProcEffectMask
--     returns 0 before any effect is examined - the proc has never fired.
--   * ProcChance is 100, so the aura is a marker whose event is casting Assault, and Chance stays 0 so
--     the loader keeps using it (SpellMgr.cpp:2122).
--   * 800090 is native: a single aura 118 on the target (target 6) with base points -21, and it is not
--     in the EffectNULL set.
--   * Assault exists eight times - 503099 through 503105 and 803108 - all SpellFamilyName 27 with
--     SpellFamilyFlags [0, 32768, 0]. The mask does NOT isolate it: counting every family-27 spell
--     holding word1 bit 32768 returns nine, because 681235 Sucker Punch shares the bit. That is also
--     why AscensionRangerAssault.cpp:23 has to pair the same flag with a DmgClass condition, a column
--     spell_proc does not have. So this row drops the mask (family 0) and lets
--     spell_ascension_spell_list_talent_proc filter by the rule's spell list, which carries the eight
--     Assault ids - the convention recorded at rev_20260921_40_ranger_dead_procs.sql:1354.
--   * ProcFlags 69904 = the four damage-by-ability classes with no auto-attack bits, SpellTypeMask 1
--     (DAMAGE) and SpellPhaseMask 2 (HIT).
--
-- The Rules entry is added in AscensionSpellListTalentProcs.h (16 -> 17).
DELETE FROM `spell_proc` WHERE `SpellId` = 704544;
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(704544, 0, 0, 0, 0, 0, 69904, 1, 2, 0, 0, 0, 0, 0, 0, 0);

DELETE FROM `spell_script_names` WHERE `spell_id` = 704544 AND `ScriptName` = 'spell_ascension_spell_list_talent_proc';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(704544, 'spell_ascension_spell_list_talent_proc');
