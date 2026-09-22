-- Hellbreaker (680216): "Casting Unleash Pestilence now removes all active movement impairing effects
-- from you."
--
-- SpellMgr::LoadSpellProcs skips a record that carries no DBC proc flags ("Skip if no proc flags in
-- DBC", src/server/game/Spells/SpellMgr.cpp), so no fallback entry is generated; Aura::GetProcEffectMask
-- then returns 0 for any aura with no proc entry ("only auras with spell proc entry can trigger proc",
-- src/server/game/Spells/Auras/SpellAuras.cpp). 680216 ships ProcFlags 0, so its aura 42 on TriggerSpell
-- 680217 has never fired.
-- The payload is authored: 680217 dispels mechanic 7 (root) and 11 (snare) through effect 108
-- (SPELL_EFFECT_DISPEL_MECHANIC) and grants immunity to both, which is exactly the tooltip.
--
-- Unleash Pestilence is a self-buff cast on the caster (801002/802606/803251 all target
-- TARGET_UNIT_CASTER), so the gate follows the self-buff shape: ProcFlags 87312
-- (PROC_FLAG_DONE_SPELL_MELEE_DMG_CLASS 16 | DONE_SPELL_RANGED_DMG_CLASS 256 |
-- DONE_SPELL_NONE_DMG_CLASS_POS 1024 | DONE_SPELL_NONE_DMG_CLASS_NEG 4096 |
-- DONE_SPELL_MAGIC_DMG_CLASS_POS 16384 | DONE_SPELL_MAGIC_DMG_CLASS_NEG 65536) with SpellTypeMask 7,
-- and SpellPhaseMask 1 (PROC_SPELL_PHASE_CAST) because the tooltip says "Casting", not "damage dealt by".
-- SpellTypeMask is never left 0 while ProcFlags carries spell bits: the loader logs an error and the proc
-- does not fire.
--
-- The family flags cannot carry the narrowing on their own: family 23 word 1 bit 16 also selects four pet
-- abilities (803253, 803254, 803693, 806963), so a mask alone would let the pet fire the talent.
-- spell_ascension_knight_of_xoroth_talent_proc holds the exact Unleash Pestilence list instead.
--
-- Chance stays 0 so the record's own ProcChance is used, per
-- rev_20260919_20_coa_proc_chance_parity.sql.
DELETE FROM `spell_proc` WHERE `SpellId` = 680216;
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(680216, 0, 0, 0, 0, 0, 87312, 7, 1, 0, 0, 0, 0, 0, 0, 0);

DELETE FROM `spell_script_names` WHERE `spell_id` = 680216 AND `ScriptName` = 'spell_ascension_knight_of_xoroth_talent_proc';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(680216, 'spell_ascension_knight_of_xoroth_talent_proc');
