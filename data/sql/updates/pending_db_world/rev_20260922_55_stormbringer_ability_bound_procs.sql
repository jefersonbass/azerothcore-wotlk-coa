-- Stormbringer and Runemaster talents whose proc auras ship without proc flags (Spell.dbc ProcFlags 0
-- and no `spell_proc` row), so the clauses below never fired. The aura-42 handler casts each record's
-- TriggerSpell (Thunder Wave 705693, Wild Steam 802637, Lightning Rod 300928). Chance stays 0
-- everywhere, deferring to each record's own ProcChance (Thunder Wave 5, Steam Conjurer 5,
-- Lightning Rod 15).
--
-- CORRECTION (2026-09-22): an earlier revision of this header claimed "Payloads are already authored
-- and native" for all three. That holds for Thunder Wave and Wild Steam but NOT for Lightning Rod:
-- 300928 carries a single effect, 169 = SPELL_EFFECT_ASCENSION_SPREAD_AURA, which the core maps to
-- &Spell::EffectNULL (SpellEffects.cpp:241). The gate below is correct and the proc does fire, but
-- 300928 delivers nothing until effect 169 is implemented - and that is a core change affecting 123
-- slots, not something this file can do. The row is kept deliberately: reverting it would return the
-- talent to "the proc never fires", and the effect-169 work would then also have to rediscover and
-- re-add this row. With the row in place the talent starts working the moment the effect lands.
-- What 300928 is meant to do, for whoever implements it: spread the Volt aura (500928, its
-- EffectTriggerSpell) from the target to every enemy the triggering Forked Lightning hit.
-- Each clause is bound to a named ability, so SpellFamilyMask keys that ability's Spell.dbc family
-- flags: Weapon Engraving: Earth word0 131072 / word1 131072; Forked Lightning word0 256 / word2 32
-- plus its second record's word2 4096, OR-ed to 4128 so either version of the ability qualifies.
-- ProcFlags 69904 = the four direct damage spell classes only ("damage dealt by <ability>");
-- SpellTypeMask 1 = PROC_SPELL_TYPE_DAMAGE, SpellPhaseMask 2 = PROC_SPELL_PHASE_HIT.
-- MASK CORRECTION (2026-09-22) - THIS SUPERSEDES THE MASK VALUES NAMED ABOVE: SpellFamilyMask
-- matches by ANY shared bit (flag96 operator& plus
-- operator bool in Util.h:513/557, consumed at SpellInfo.cpp:1439), so a mask set to an ability full
-- flags also matched every ability sharing any of those bits - word2 bit 32 alone is shared by about
-- thirteen Stormbringer abilities. The rows here now carry only the bits that are EXCLUSIVE to the
-- ability the clause names, so the mask selects it alone. Where an ability has no exclusive bit at
-- all (its whole flag set is shared), the mask cannot isolate it and the row must be filtered by the
-- spell list in AscensionStormbringerRunemasterTalentProcs instead.
DELETE FROM `spell_proc` WHERE `SpellId` IN (705692, 805743, 300609);
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(705692, 0, 22, 256, 0, 4096, 69904, 1, 2, 0, 0, 0, 0, 0, 0, 0),
(805743, 0, 38, 0, 131072, 0, 69904, 1, 2, 0, 0, 0, 0, 0, 0, 0),
(300609, 0, 22, 256, 0, 4096, 69904, 1, 2, 0, 0, 0, 0, 0, 0, 0);
