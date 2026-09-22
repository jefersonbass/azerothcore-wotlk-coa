-- Plague Horde (804246): "Increases the duration of the Rotlings created by your Animate: Rotling spell
-- by 50%. In addition, dealing Periodic Damage to an enemy now has a small chance to summon a Rotling."
--
-- SpellMgr::LoadSpellProcs skips a record that carries no DBC proc flags ("Skip if no proc flags in
-- DBC", src/server/game/Spells/SpellMgr.cpp), so no fallback entry is generated; Aura::GetProcEffectMask
-- then returns 0 for any aura with no proc entry ("only auras with spell proc entry can trigger proc",
-- src/server/game/Spells/Auras/SpellAuras.cpp). 804246 ships ProcFlags 0, so its effect 2 - aura 42 on
-- TriggerSpell 805977 - has never fired.
-- The payload is authored: 805977 is the Rotling summon. Effects 0 and 1 are already live - an aura 108
-- carrying SPELLMOD_DURATION and an aura 107 carrying SPELLMOD_COST, both keyed to the Rotling family bit
-- (word 1, bit 5) - which is the tooltip's +50% duration. Only the proc gate was missing.
-- Same defect and same shape as rev_20260920_48_bloodmage_ultra_instinct_proc.sql.
--
-- ProcFlags 262144 (PROC_FLAG_DONE_PERIODIC) - the clause's trigger is periodic damage, so the direct
--   melee/ranged/spell flags are deliberately omitted.
-- SpellTypeMask 1 (PROC_SPELL_TYPE_DAMAGE) - Unit::ProcSkillsAndAuras computes DAMAGE only when the event
--   carries a DamageInfo with damage or absorb. It is never 0 while ProcFlags carries spell bits.
-- SpellPhaseMask 2 (PROC_SPELL_PHASE_HIT) - a periodic tick resolves at the HIT phase.
-- HitMask 0 (PROC_HIT_NONE) - any tick, not only a critical one.
-- Chance stays 0 so the record's own ProcChance (15) is used, per
-- rev_20260919_20_coa_proc_chance_parity.sql.
DELETE FROM `spell_proc` WHERE `SpellId` = 804246;
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(804246, 0, 0, 0, 0, 0, 262144, 1, 2, 0, 0, 0, 0, 0, 0, 0);
