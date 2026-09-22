-- Stormbringer and Runemaster talents whose proc auras ship without proc flags (Spell.dbc ProcFlags 0
-- and no `spell_proc` row), so the clauses below never fired. Payloads are already authored and native:
-- the aura-42 handler casts each record's TriggerSpell (Conduction 567560, Leyline Magician 520146,
-- Elemental Inscription 706522, Master Airbender 680878/6801879/681010). Chance stays 0 everywhere,
-- deferring to each record's own ProcChance (Shockingly Powerful 100, Leyline Magician 100,
-- Elemental Inscription 100, Master Airbender 100).
-- SpellPhaseMask follows the tooltip verb, as in rev_20260919_64: a "Casting <ability>" clause fires on
-- PROC_SPELL_PHASE_CAST (1) so it raises once per cast rather than once per target hit, while a
-- "critical strikes with <ability>" clause fires on PROC_SPELL_PHASE_HIT (2) with HitMask 2
-- (PROC_HIT_CRITICAL). Each clause names one ability, so SpellFamilyMask keys that ability's own
-- Spell.dbc family flags: Torrential Wrath word1 1 / word2 32, Primordial Blast 4194304/1048576/64,
-- Smolder word0 512, Gale 16384/537001984/136.
-- ProcFlags 69972 = the four direct damage spell classes plus melee and ranged auto attacks;
-- 69904 = those four spell classes only. SpellTypeMask 1 = PROC_SPELL_TYPE_DAMAGE.
DELETE FROM `spell_proc` WHERE `SpellId` IN (572310, 706823, 806737, 705719);
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(572310, 0, 22, 0, 1, 32, 69972, 1, 2, 2, 0, 0, 0, 0, 0, 0),
(706823, 0, 38, 4194304, 1048576, 64, 69904, 1, 1, 0, 0, 0, 0, 0, 0, 0),
(806737, 0, 38, 512, 0, 0, 69904, 1, 1, 0, 0, 0, 0, 0, 0, 0),
(705719, 0, 22, 16384, 537001984, 136, 69904, 1, 1, 0, 0, 0, 0, 0, 0, 0);
