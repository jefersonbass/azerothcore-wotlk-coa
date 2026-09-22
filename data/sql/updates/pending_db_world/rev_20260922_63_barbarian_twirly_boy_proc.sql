-- Twirly Boy (705199): "Critical strikes deal bonus damage and extend the duration of Axe Twirling
-- while it is active."
--
-- SpellMgr::LoadSpellProcs skips a record that carries no DBC proc flags ("Skip if no proc flags in
-- DBC", src/server/game/Spells/SpellMgr.cpp), so no fallback entry is generated; Aura::GetProcEffectMask
-- then returns 0 for any aura with no proc entry ("only auras with spell proc entry can trigger proc",
-- src/server/game/Spells/Auras/SpellAuras.cpp). 705199 ships ProcFlags 0, so its aura 42 on TriggerSpell
-- 520031 has never fired.
-- The payload is authored: 520031 carries Ascension effect 177 with MiscValue 800401 (Axe Twirling) and
-- BasePoints 1999, plus effect 2 (SPELL_EFFECT_SCHOOL_DAMAGE) BasePoints 136. Only the gate is missing.
-- Same defect and same shape as rev_20260920_48_bloodmage_ultra_instinct_proc.sql.
--
-- ProcFlags 69972 = PROC_FLAG_DONE_MELEE_AUTO_ATTACK (4) | PROC_FLAG_DONE_SPELL_MELEE_DMG_CLASS (16) |
--   PROC_FLAG_DONE_RANGED_AUTO_ATTACK (64) | PROC_FLAG_DONE_SPELL_RANGED_DMG_CLASS (256) |
--   PROC_FLAG_DONE_SPELL_NONE_DMG_CLASS_NEG (4096) | PROC_FLAG_DONE_SPELL_MAGIC_DMG_CLASS_NEG (65536).
--   Main-hand/off-hand flags are left out on purpose: they are set alongside
--   PROC_FLAG_DONE_MELEE_AUTO_ATTACK and would double the events for one swing. Periodic is excluded
--   because the tooltip's clause is "critical strikes" on damage this talent can extend, and the trigger
--   pays a direct SCHOOL_DAMAGE hit.
-- SpellTypeMask 1 (PROC_SPELL_TYPE_DAMAGE) - Unit::ProcSkillsAndAuras computes DAMAGE only when the event
--   carries a DamageInfo with damage or absorb.
-- SpellPhaseMask 2 (PROC_SPELL_PHASE_HIT) - crits resolve at the HIT phase.
-- HitMask 2 (PROC_HIT_CRITICAL) - "critical strikes" only; CanSpellTriggerProcOnEvent requires the event's
--   hit mask to contain this bit (SpellMgr.cpp ~950).
-- Chance stays 0 so the record's own ProcChance (100) is used, per
-- rev_20260919_20_coa_proc_chance_parity.sql.
DELETE FROM `spell_proc` WHERE `SpellId` = 705199;
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(705199, 0, 0, 0, 0, 0, 69972, 1, 2, 2, 0, 0, 0, 0, 0, 0);
