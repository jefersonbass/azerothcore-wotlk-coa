-- Bloodthirsty Rage (801766): "Enrage Effect - Your melee auto attacks now have a 10% chance to enrage
-- you for 8 sec, increasing your damage."
--
-- SpellMgr::LoadSpellProcs skips a record that carries no DBC proc flags ("Skip if no proc flags in
-- DBC", src/server/game/Spells/SpellMgr.cpp), so no fallback entry is generated; Aura::GetProcEffectMask
-- then returns 0 for any aura with no proc entry ("only auras with spell proc entry can trigger proc",
-- src/server/game/Spells/Auras/SpellAuras.cpp). 801766 ships ProcFlags 0, so its aura 42
-- (SPELL_AURA_PROC_TRIGGER_SPELL) on TriggerSpell 680851 has never fired.
-- The payload is authored: 680851 carries aura 79 (SPELL_AURA_MOD_DAMAGE_PERCENT_DONE) BasePoints 4 /
-- DieSides 1 -> +5% damage for duration index 31. Only the gate is missing. Same defect and same shape as
-- rev_20260920_48_bloodmage_ultra_instinct_proc.sql.
--
-- ProcFlags 4 (PROC_FLAG_DONE_MELEE_AUTO_ATTACK) - the tooltip names melee auto attacks and nothing else,
--   so the ranged and spell flags are omitted. Main-hand/off-hand flags are left out on purpose: they are
--   set alongside PROC_FLAG_DONE_MELEE_AUTO_ATTACK and would double the events for one swing.
-- SpellTypeMask 1 (PROC_SPELL_TYPE_DAMAGE) - Unit::ProcSkillsAndAuras computes DAMAGE only when the event
--   carries a DamageInfo with damage or absorb.
-- SpellPhaseMask 2 (PROC_SPELL_PHASE_HIT).
-- HitMask 0 (PROC_HIT_NONE) - the tooltip asks for any auto attack, not a critical one.
-- Chance stays 0 so the record's own ProcChance (10) is used, per
-- rev_20260919_20_coa_proc_chance_parity.sql.
DELETE FROM `spell_proc` WHERE `SpellId` = 801766;
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(801766, 0, 0, 0, 0, 0, 4, 1, 2, 0, 0, 0, 0, 0, 0, 0);
