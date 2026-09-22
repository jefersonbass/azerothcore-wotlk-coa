-- Eye of the Tornado (706297): "Your damaging critical strikes now refund 5 Static."
--
-- SpellMgr::LoadSpellProcs skips a record that carries no DBC proc flags ("Skip if no proc flags in
-- DBC", src/server/game/Spells/SpellMgr.cpp), so no fallback entry is generated; Aura::GetProcEffectMask
-- then returns 0 for any aura with no proc entry ("only auras with spell proc entry can trigger proc",
-- src/server/game/Spells/Auras/SpellAuras.cpp). 706297 ships ProcFlags 0, so its single effect - aura 42
-- (SPELL_AURA_PROC_TRIGGER_SPELL) on TriggerSpell 804044 "Add 5 Static" - has never fired.
-- The payload is authored: 804044 pays the 5 Static through Ascension effect 175 (misc 5). Only the gate
-- is missing. Same defect and same shape as rev_20260920_48_bloodmage_ultra_instinct_proc.sql.
--
-- ProcFlags 332116 = PROC_FLAG_DONE_MELEE_AUTO_ATTACK (4) | PROC_FLAG_DONE_SPELL_MELEE_DMG_CLASS (16) |
--   PROC_FLAG_DONE_RANGED_AUTO_ATTACK (64) | PROC_FLAG_DONE_SPELL_RANGED_DMG_CLASS (256) |
--   PROC_FLAG_DONE_SPELL_NONE_DMG_CLASS_NEG (4096) | PROC_FLAG_DONE_SPELL_MAGIC_DMG_CLASS_NEG (65536) |
--   PROC_FLAG_DONE_PERIODIC (262144). Main-hand/off-hand flags are left out on purpose: they are set
--   alongside PROC_FLAG_DONE_MELEE_AUTO_ATTACK and would double the events for one swing. Periodic is
--   included because the tooltip says "damaging critical strikes", not "direct".
-- SpellTypeMask 1 (PROC_SPELL_TYPE_DAMAGE) - Unit::ProcSkillsAndAuras computes DAMAGE only when the event
--   carries a DamageInfo with damage or absorb.
-- SpellPhaseMask 2 (PROC_SPELL_PHASE_HIT) - crits resolve at the HIT phase.
-- HitMask 2 (PROC_HIT_CRITICAL) - "critical strikes" only; CanSpellTriggerProcOnEvent requires the event's
--   hit mask to contain this bit (SpellMgr.cpp ~950).
-- Chance stays 0 so the record's own ProcChance (100) is used, per
-- rev_20260919_20_coa_proc_chance_parity.sql.
DELETE FROM `spell_proc` WHERE `SpellId` = 706297;
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(706297, 0, 0, 0, 0, 0, 332116, 1, 2, 2, 0, 0, 0, 0, 0, 0);
