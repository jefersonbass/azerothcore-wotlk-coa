-- Smolder (805700): "Dealing direct Fire Damage now ignites targets, Burning them for an additional 14
-- to 16 Fire Damage over 4 sec."
--
-- SpellMgr::LoadSpellProcs skips a record that carries no DBC proc flags ("Skip if no proc flags in
-- DBC", src/server/game/Spells/SpellMgr.cpp), so no fallback entry is generated; Aura::GetProcEffectMask
-- then returns 0 for any aura with no proc entry ("only auras with spell proc entry can trigger proc",
-- src/server/game/Spells/Auras/SpellAuras.cpp). 805700 ships ProcFlags 0, so its aura 42
-- (SPELL_AURA_PROC_TRIGGER_SPELL) on TriggerSpell 805701 has never fired.
-- The payload is authored: 805701 carries aura 3 (SPELL_AURA_PERIODIC_DAMAGE) BasePoints 6 / DieSides 1
-- for duration index 35. Only the gate is missing. Same defect and same shape as
-- rev_20260920_48_bloodmage_ultra_instinct_proc.sql.
--
-- ProcFlags 69972 = PROC_FLAG_DONE_MELEE_AUTO_ATTACK (4) | PROC_FLAG_DONE_SPELL_MELEE_DMG_CLASS (16) |
--   PROC_FLAG_DONE_RANGED_AUTO_ATTACK (64) | PROC_FLAG_DONE_SPELL_RANGED_DMG_CLASS (256) |
--   PROC_FLAG_DONE_SPELL_NONE_DMG_CLASS_NEG (4096) | PROC_FLAG_DONE_SPELL_MAGIC_DMG_CLASS_NEG (65536).
--   The tooltip's "direct Fire Damage" needs every direct damage source, so the full set is used;
--   main-hand/off-hand flags are left out on purpose because they are set alongside
--   PROC_FLAG_DONE_MELEE_AUTO_ATTACK and would double the events for one swing. Periodic is excluded by
--   "direct".
-- SchoolMask 4 (SPELL_SCHOOL_MASK_FIRE) - CanSpellTriggerProcOnEvent tests the event's spell school
--   against SchoolMask (SpellMgr.cpp ~950), so this keeps the proc to Fire Damage as the tooltip states.
-- SpellTypeMask 1 (PROC_SPELL_TYPE_DAMAGE) - Unit::ProcSkillsAndAuras computes DAMAGE only when the event
--   carries a DamageInfo with damage or absorb.
-- SpellPhaseMask 2 (PROC_SPELL_PHASE_HIT).
-- HitMask 0 (PROC_HIT_NONE) - the tooltip asks for any Fire damage, not a critical one.
-- Chance stays 0 so the record's own ProcChance is used, per
-- rev_20260919_20_coa_proc_chance_parity.sql.
DELETE FROM `spell_proc` WHERE `SpellId` = 805700;
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(805700, 4, 0, 0, 0, 0, 69972, 1, 2, 0, 0, 0, 0, 0, 0, 0);
