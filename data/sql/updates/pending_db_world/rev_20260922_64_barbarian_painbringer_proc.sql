-- Painbringer (706286): "Dealing damage with Smash now increases the damage of your next Brutal Swing
-- within 8 sec by 25%."
--
-- SpellMgr::LoadSpellProcs skips a record that carries no DBC proc flags ("Skip if no proc flags in
-- DBC", src/server/game/Spells/SpellMgr.cpp), so no fallback entry is generated; Aura::GetProcEffectMask
-- then returns 0 for any aura with no proc entry ("only auras with spell proc entry can trigger proc",
-- src/server/game/Spells/Auras/SpellAuras.cpp). 706286 ships ProcFlags 0, so its aura 42 on TriggerSpell
-- 706485 has never fired.
-- The payload is authored: 706485 is the 25% Brutal Swing modifier for duration index 31. Only the gate
-- is missing. Same defect and same shape as rev_20260920_48_bloodmage_ultra_instinct_proc.sql.
--
-- SpellFamilyName 18 (Barbarian) with SpellFamilyMask0 0x10000000 (word 0, bit 28) selects Smash and
-- nothing else: all 20 family-18 records carrying that bit are Smash ranks. AscensionBarbarianCompletion
-- already documents that same bit as Smash's when it rekeys Breaking Morale.
-- ProcFlags 69904 (PROC_FLAG_DONE_SPELL_MELEE_DMG_CLASS 16 | DONE_SPELL_RANGED_DMG_CLASS 256 |
--   DONE_SPELL_NONE_DMG_CLASS_NEG 4096 | DONE_SPELL_MAGIC_DMG_CLASS_NEG 65536) is the spell-only set: the
--   clause names an ability, so auto attacks are deliberately left out.
-- SpellTypeMask 1 (PROC_SPELL_TYPE_DAMAGE) - Unit::ProcSkillsAndAuras computes DAMAGE only when the event
--   carries a DamageInfo with damage or absorb. It is never 0 while ProcFlags carries spell bits: the
--   loader logs an error and the proc does not fire.
-- SpellPhaseMask 2 (PROC_SPELL_PHASE_HIT) - the clause is "damage dealt", not "casting".
-- HitMask 0 (PROC_HIT_NONE) - any hit, not only a critical one.
-- Chance stays 0 so the record's own ProcChance (100) is used, per
-- rev_20260919_20_coa_proc_chance_parity.sql.
DELETE FROM `spell_proc` WHERE `SpellId` = 706286;
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(706286, 0, 18, 268435456, 0, 0, 69904, 1, 2, 0, 0, 0, 0, 0, 0, 0);
