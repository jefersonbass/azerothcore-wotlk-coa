-- Hailfire (706283), issue #3052: "Gives Rapid Barrage hits a 15% chance to generate a stack of Advantage."
--
-- Measured before writing:
--   * 706283 carries a single effect, a live SPELL_AURA_PROC_TRIGGER_SPELL (aura 42) on 804329 Advantage.
--     Spell.dbc gives the record ProcFlags 0 and the world DB holds no `spell_proc` row, so
--     SpellMgr::LoadSpellProcs skips it ("Skip if no proc flags in DBC") and Aura::GetProcEffectMask
--     returns 0 before any effect is examined - the proc has never fired. Same shape as the rows in
--     rev_20260921_40_ranger_dead_procs.sql.
--   * ProcChance is 15, so this is a real chance proc rather than a marker, and Chance stays 0 so the
--     loader falls back to that value (SpellMgr.cpp:2122).
--   * 804329 Advantage is native and needs no work: aura 108, 107 and 108 with base points 49, 3999 and
--     29, all targeting the caster, and none of its effects is in the EffectNULL set.
--   * Rapid Barrage exists twice, 500072 and 804276, and both carry SpellFamilyName 0 with
--     SpellFamilyFlags [0, 0, 0] - no exclusive bit at all, so SpellFamilyMask cannot isolate the
--     ability and the row cannot be keyed by a mask. This follows the convention recorded in
--     rev_20260921_40_ranger_dead_procs.sql:1354 for the same situation: "both rows drop the mask
--     (family 0) and aura_ascension_spell_list_talent_proc filters" by the rule's spell list. The Rules
--     entry for 706283 carries both Rapid Barrage ids.
--   * ProcFlags 69904 = DONE_SPELL_MELEE_DMG_CLASS | DONE_SPELL_RANGED_DMG_CLASS |
--     DONE_SPELL_NONE_DMG_CLASS_NEG | DONE_SPELL_MAGIC_DMG_CLASS_NEG - damage dealt by abilities, no
--     auto-attack bits, which is what "Rapid Barrage hits" means. SpellTypeMask 1 (DAMAGE) and
--     SpellPhaseMask 2 (HIT) complete it.
--
-- The Rules entry is added in AscensionSpellListTalentProcs.h (15 -> 16).
DELETE FROM `spell_proc` WHERE `SpellId` = 706283;
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(706283, 0, 0, 0, 0, 0, 69904, 1, 2, 0, 0, 0, 0, 0, 0, 0);

DELETE FROM `spell_script_names` WHERE `spell_id` = 706283 AND `ScriptName` = 'spell_ascension_spell_list_talent_proc';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(706283, 'spell_ascension_spell_list_talent_proc');
