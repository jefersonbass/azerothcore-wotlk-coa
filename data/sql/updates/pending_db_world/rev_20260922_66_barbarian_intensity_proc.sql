-- Intensity (705226): "Dealing damage with Berserker Axe now reduces the target's Armor by 4 for 20 sec."
--
-- SpellMgr::LoadSpellProcs skips a record that carries no DBC proc flags ("Skip if no proc flags in
-- DBC", src/server/game/Spells/SpellMgr.cpp), so no fallback entry is generated; Aura::GetProcEffectMask
-- then returns 0 for any aura with no proc entry ("only auras with spell proc entry can trigger proc",
-- src/server/game/Spells/Auras/SpellAuras.cpp). 705226 ships ProcFlags 0, so its aura 42 on TriggerSpell
-- 807179 has never fired.
-- The payload is authored: 807179 carries the flat armor reduction for duration index 31. Only the gate is
-- missing. Same defect and same shape as rev_20260920_48_bloodmage_ultra_instinct_proc.sql.
--
-- SpellFamilyName 18 (Barbarian) with SpellFamilyMask1 0x80000 (word 1, bit 19) selects Berserker Axe.
-- KNOWN COLLATERAL, accepted: the same bit is also carried by Spite, so damage with Spite triggers Intensity
-- too. The alternative is a talent-proc script holding the exact Berserker Axe ranks (503408-503414); this
-- row is the cheaper half of that trade and the over-grant stays inside one player ability, unlike the
-- Hellbreaker case where the collateral was four PET abilities acting on the player's behalf. Revisit if
-- Spite ever becomes a real damage source.
-- ProcFlags 69904 (PROC_FLAG_DONE_SPELL_MELEE_DMG_CLASS 16 | DONE_SPELL_RANGED_DMG_CLASS 256 |
--   DONE_SPELL_NONE_DMG_CLASS_NEG 4096 | DONE_SPELL_MAGIC_DMG_CLASS_NEG 65536) is the spell-only set: the
--   clause names an ability, so auto attacks are deliberately left out.
-- SpellTypeMask 1 (PROC_SPELL_TYPE_DAMAGE) - Unit::ProcSkillsAndAuras computes DAMAGE only when the event
--   carries a DamageInfo with damage or absorb. It is never 0 while ProcFlags carries spell bits.
-- SpellPhaseMask 2 (PROC_SPELL_PHASE_HIT) - the clause is "damage dealt", not "casting".
-- HitMask 0 (PROC_HIT_NONE) - any hit, not only a critical one.
-- Chance stays 0 so the record's own ProcChance (100) is used, per
-- rev_20260919_20_coa_proc_chance_parity.sql.
DELETE FROM `spell_proc` WHERE `SpellId` = 705226;
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(705226, 0, 18, 0, 524288, 0, 69904, 1, 2, 0, 0, 0, 0, 0, 0, 0);
