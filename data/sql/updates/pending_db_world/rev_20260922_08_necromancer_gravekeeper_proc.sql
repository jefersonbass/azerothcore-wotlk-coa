-- Gravekeeper (704698): "Periodic damage now has a 10% chance to animate a Lesser Zombie to aid you in
-- combat for 10 sec. Can only occur once every 30 sec."
--
-- SpellMgr::LoadSpellProcs skips a record that carries no DBC proc flags ("Skip if no proc flags in
-- DBC", src/server/game/Spells/SpellMgr.cpp), so no fallback entry is generated; Aura::GetProcEffectMask
-- then returns 0 for any aura with no proc entry ("only auras with spell proc entry can trigger proc",
-- src/server/game/Spells/Auras/SpellAuras.cpp). 704698 ships ProcFlags 0, so its aura 42
-- (SPELL_AURA_PROC_TRIGGER_SPELL) on TriggerSpell 504313 has never fired.
-- The payload is authored: 504313 is the necromancer summon entry already bound to
-- spell_ascension_necromancer_summon (rev_20260909_01_necromancer_completion.sql) and driven by the
-- NecromancerSummons table. Only the gate is missing. Same defect and same shape as
-- rev_20260920_48_bloodmage_ultra_instinct_proc.sql.
--
-- ProcFlags 262144 (PROC_FLAG_DONE_PERIODIC) - the tooltip's trigger is periodic damage, so the direct
--   melee/ranged/spell flags are omitted.
-- SpellTypeMask 1 (PROC_SPELL_TYPE_DAMAGE) - Unit::ProcSkillsAndAuras computes DAMAGE only when the event
--   carries a DamageInfo with damage or absorb.
-- SpellPhaseMask 2 (PROC_SPELL_PHASE_HIT).
-- HitMask 0 (PROC_HIT_NONE) - the tooltip asks for any periodic tick, not a critical one.
-- Cooldown 30 supplies the tooltip's "once every 30 sec"; the DBC carries no internal cooldown for it.
-- Chance stays 0 so the record's own ProcChance (10) is used, per
-- rev_20260919_20_coa_proc_chance_parity.sql.
DELETE FROM `spell_proc` WHERE `SpellId` = 704698;
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(704698, 0, 0, 0, 0, 0, 262144, 1, 2, 0, 0, 0, 0, 0, 30, 0);
