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
-- MASK CORRECTION (2026-09-22) - THIS SUPERSEDES THE MASK VALUES NAMED ABOVE: SpellFamilyMask
-- matches by ANY shared bit (flag96 operator& plus
-- operator bool in Util.h:513/557, consumed at SpellInfo.cpp:1439), so a mask set to an ability full
-- flags also matched every ability sharing any of those bits - word2 bit 32 alone is shared by about
-- thirteen Stormbringer abilities. The rows here now carry only the bits that are EXCLUSIVE to the
-- ability the clause names, so the mask selects it alone. Where an ability has no exclusive bit at
-- all (its whole flag set is shared), the mask cannot isolate it and the row must be filtered by the
-- spell list in AscensionStormbringerRunemasterTalentProcs instead.
-- ROW REMOVAL (2026-09-24) - THIS SUPERSEDES THE 705719 ROW THIS FILE USED TO WRITE: it was restored
-- to the shape its own gameplay scenario asserts, by rev_20260924_03 (which sorts after this file).
-- stormbringer-fix-on-cast-proc-cooldowns-master-airbender.json asserts a negative - "Arm of Thorim is
-- outside Gale's 0x4000 mask" - so the row must carry Gale's 0x4000 alone; this file added
-- SpellFamilyMask1 536870912 and SpellFamilyMask2 128 on top of it.
-- The basis is the scenario assertion, not an independent measurement.
DELETE FROM `spell_proc` WHERE `SpellId` IN (572310, 706823, 806737);
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(572310, 0, 22, 0, 1, 0, 69972, 1, 2, 2, 0, 0, 0, 0, 0, 0),
(706823, 0, 38, 0, 0, 0, 69904, 1, 1, 0, 0, 0, 0, 0, 0, 0),
(806737, 0, 38, 0, 0, 0, 69904, 1, 1, 0, 0, 0, 0, 0, 0, 0);

DELETE FROM `spell_script_names` WHERE `spell_id` IN (706823, 806737)
  AND `ScriptName` = 'spell_ascension_stormbringer_runemaster_talent_proc';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(706823, 'spell_ascension_stormbringer_runemaster_talent_proc'),
(806737, 'spell_ascension_stormbringer_runemaster_talent_proc');
