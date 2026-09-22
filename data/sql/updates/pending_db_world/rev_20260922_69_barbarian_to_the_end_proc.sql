-- To The End (801770): "Taking damage while at or below 35% of your maximum health now regenerates 3%
-- of your maximum health, repeating every 3.00 sec, reduces your Energy costs by 40%, and increases your
-- damage dealt by 10% for 9 sec. Can only occur once every 2 min."
--
-- SpellMgr::LoadSpellProcs skips a record that carries no DBC proc flags ("Skip if no proc flags in
-- DBC", src/server/game/Spells/SpellMgr.cpp), so no fallback entry is generated; Aura::GetProcEffectMask
-- then returns 0 for any aura with no proc entry ("only auras with spell proc entry can trigger proc",
-- src/server/game/Spells/Auras/SpellAuras.cpp). 801770 ships ProcFlags 0, so its aura 42 on TriggerSpell
-- 801771 has never fired.
-- The payload is authored: 801771 carries aura 72 (MiscValue 127), aura 79 (BasePoints 9, MiscValue 127)
-- and aura 20 for duration index 105, which is the tooltip's regeneration, cost reduction and damage
-- bonus. Only the gate and the health condition were missing.
--
-- ProcFlags 1048576 (PROC_FLAG_TAKEN_DAMAGE) - the clause is "taking damage", which is a TAKEN event on
--   the victim, not a DONE one. This is the catch-all TAKEN flag, so every damage school qualifies.
-- SpellTypeMask 1 (PROC_SPELL_TYPE_DAMAGE) - Unit::ProcSkillsAndAuras computes DAMAGE only when the event
--   carries a DamageInfo with damage or absorb. It is never 0 while ProcFlags carries spell bits: the
--   loader logs an error and the proc does not fire.
-- SpellPhaseMask 2 (PROC_SPELL_PHASE_HIT).
-- HitMask 0 (PROC_HIT_NONE) - any hit, not only a critical one.
-- Cooldown 120 carries the tooltip's "once every 2 min", which the DBC does not encode.
--
-- The "at or below 35%" half cannot be expressed by any ProcFlag, so aura_barbarian_to_the_end checks the
-- caster's health percentage in DoCheckProc and rejects the proc above the threshold.
-- Chance stays 0 so the record's own ProcChance is used, per
-- rev_20260919_20_coa_proc_chance_parity.sql.
DELETE FROM `spell_proc` WHERE `SpellId` = 801770;
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(801770, 0, 0, 0, 0, 0, 1048576, 1, 2, 0, 0, 0, 0, 0, 120, 0);

DELETE FROM `spell_script_names` WHERE `spell_id` = 801770 AND `ScriptName` = 'aura_barbarian_to_the_end';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(801770, 'aura_barbarian_to_the_end');
