-- Thunder Grip (301219): "Direct damage dealt now has a 30% chance to apply Thunder Grip to enemies."
--
-- SpellMgr::LoadSpellProcs skips a record that carries no DBC proc flags ("Skip if no proc flags in
-- DBC", src/server/game/Spells/SpellMgr.cpp), so no fallback entry is generated; Aura::GetProcEffectMask
-- then returns 0 for any aura with no proc entry ("only auras with spell proc entry can trigger proc",
-- src/server/game/Spells/Auras/SpellAuras.cpp). 301219 ships ProcFlags 0, so its single effect - aura 42
-- (SPELL_AURA_PROC_TRIGGER_SPELL) on TriggerSpell 301220 - has never fired.
-- The payload is authored: 301220 applies aura 92 (SPELL_AURA_MOD_DECREASE_SPEED) BasePoints -21 /
-- DieSides 1 -> -20% movement speed and aura 191, for duration index 31. Only the gate is missing.
-- Same defect and same shape as rev_20260920_48_bloodmage_ultra_instinct_proc.sql.
--
-- ProcFlags 69972 = PROC_FLAG_DONE_MELEE_AUTO_ATTACK (4) | PROC_FLAG_DONE_SPELL_MELEE_DMG_CLASS (16) |
--   PROC_FLAG_DONE_RANGED_AUTO_ATTACK (64) | PROC_FLAG_DONE_SPELL_RANGED_DMG_CLASS (256) |
--   PROC_FLAG_DONE_SPELL_NONE_DMG_CLASS_NEG (4096) | PROC_FLAG_DONE_SPELL_MAGIC_DMG_CLASS_NEG (65536).
--   Main-hand/off-hand flags are left out on purpose: they are set alongside
--   PROC_FLAG_DONE_MELEE_AUTO_ATTACK and would double the events for one swing. Periodic is excluded by
--   the tooltip's "direct".
-- SpellTypeMask 1 (PROC_SPELL_TYPE_DAMAGE) - Unit::ProcSkillsAndAuras computes DAMAGE only when the event
--   carries a DamageInfo with damage or absorb.
-- SpellPhaseMask 2 (PROC_SPELL_PHASE_HIT) - the event resolves at the HIT phase.
-- HitMask 0 (PROC_HIT_NONE) - the tooltip asks for any damaging hit, not a critical one.
-- Chance stays 0 so the record's own ProcChance (30) is used, per
-- rev_20260919_20_coa_proc_chance_parity.sql.
DELETE FROM `spell_proc` WHERE `SpellId` = 301219;
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(301219, 0, 0, 0, 0, 0, 69972, 1, 2, 0, 0, 0, 0, 0, 0, 0);
