-- Abyssal Protection (801967): "Increases your Stamina by 6% and reduces all magic damage taken by 4%.
-- Taking melee damage has a 40% chance to heal allies within 15 yd of you for 34, scaling with Intellect
-- and spell power. This effect cannot occur more than once every 10 sec."
--
-- SpellMgr::LoadSpellProcs skips a record that carries no DBC proc flags ("Skip if no proc flags in
-- DBC", src/server/game/Spells/SpellMgr.cpp), so no fallback entry is generated; Aura::GetProcEffectMask
-- then returns 0 for any aura with no proc entry ("only auras with spell proc entry can trigger proc",
-- src/server/game/Spells/Auras/SpellAuras.cpp). 801967 ships ProcFlags 0, so its aura 42
-- (SPELL_AURA_PROC_TRIGGER_SPELL, effect 1) on TriggerSpell 801936 has never fired.
-- The payload is authored: 801936 heals through effect 10 (SPELL_EFFECT_HEAL) BasePoints 3. Only the gate
-- is missing. Same defect and same shape as rev_20260920_48_bloodmage_ultra_instinct_proc.sql.
--
-- ProcFlags 8 (PROC_FLAG_TAKEN_MELEE_AUTO_ATTACK) - the tooltip's trigger is damage taken from a melee
--   attack, which is a TAKEN event, not a DONE one. ProcSkillsAndAuras sets PROC_FLAG_TAKEN_* on the
--   victim from the same DamageInfo it uses for the attacker's DONE flags.
-- SpellTypeMask 1 (PROC_SPELL_TYPE_DAMAGE) - Unit::ProcSkillsAndAuras computes DAMAGE only when the event
--   carries a DamageInfo with damage or absorb.
-- SpellPhaseMask 2 (PROC_SPELL_PHASE_HIT).
-- HitMask 0 (PROC_HIT_NONE) - the tooltip asks for any melee hit, not a critical one.
-- Cooldown 10 supplies the tooltip's "cannot occur more than once every 10 sec".
-- Chance stays 0 so the record's own ProcChance (40) is used, per
-- rev_20260919_20_coa_proc_chance_parity.sql.
DELETE FROM `spell_proc` WHERE `SpellId` = 801967;
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(801967, 0, 0, 0, 0, 0, 8, 1, 2, 0, 0, 0, 0, 0, 10, 0);
