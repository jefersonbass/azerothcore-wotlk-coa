-- Ancestor's Wrath (801765): "Dealing damage with Ancestral Strike and Brutal Swing now reduces an
-- enemy's Armor by 4% for 20 sec, stacking 5 times."
--
-- SpellMgr::LoadSpellProcs skips a record that carries no DBC proc flags ("Skip if no proc flags in
-- DBC", src/server/game/Spells/SpellMgr.cpp), so no fallback entry is generated; Aura::GetProcEffectMask
-- then returns 0 for any aura with no proc entry ("only auras with spell proc entry can trigger proc",
-- src/server/game/Spells/Auras/SpellAuras.cpp). 801765 ships ProcFlags 0, so its aura 42 on TriggerSpell
-- 560934 has never fired.
-- The payload is authored: 560934 carries the 4% armor reduction for duration index 31. Only the gate is
-- missing. Same defect and same shape as rev_20260920_48_bloodmage_ultra_instinct_proc.sql.
--
-- SpellFamilyName 18 (Barbarian) with SpellFamilyMask1 0x100002 covers both abilities the clause names:
-- word 1 bit 20 is Ancestral Strike (AscensionBarbarianCompletion documents that bit as Ancestral
-- Strike's) and word 1 bit 1 is Brutal Swing. Each bit is exclusive in family 18 - all 9 records carrying
-- bit 20 are Ancestral Strike ranks and all 8 carrying bit 1 are Brutal Swing ranks - so the OR of the two
-- selects exactly the pair and nothing else.
-- ProcFlags 69904 (PROC_FLAG_DONE_SPELL_MELEE_DMG_CLASS 16 | DONE_SPELL_RANGED_DMG_CLASS 256 |
--   DONE_SPELL_NONE_DMG_CLASS_NEG 4096 | DONE_SPELL_MAGIC_DMG_CLASS_NEG 65536) is the spell-only set: the
--   clause names abilities, so auto attacks are deliberately left out.
-- SpellTypeMask 1 (PROC_SPELL_TYPE_DAMAGE) - Unit::ProcSkillsAndAuras computes DAMAGE only when the event
--   carries a DamageInfo with damage or absorb. It is never 0 while ProcFlags carries spell bits.
-- SpellPhaseMask 2 (PROC_SPELL_PHASE_HIT) - the clause is "damage dealt", not "casting".
-- HitMask 0 (PROC_HIT_NONE) - any hit, not only a critical one.
-- Chance stays 0 so the record's own ProcChance (100) is used, per
-- rev_20260919_20_coa_proc_chance_parity.sql.
DELETE FROM `spell_proc` WHERE `SpellId` = 801765;
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(801765, 0, 18, 0, 1048578, 0, 69904, 1, 2, 0, 0, 0, 0, 0, 0, 0);
