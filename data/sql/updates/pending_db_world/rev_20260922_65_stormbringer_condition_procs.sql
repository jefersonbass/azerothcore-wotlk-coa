-- Two Stormbringer talents whose proc auras ship without proc flags (Spell.dbc ProcFlags 0 and no
-- `spell_proc` row), so the clauses below never fired. Both payloads are already authored and native,
-- so no script is needed - the row's mask carries the condition. Chance stays 0, deferring to each
-- record's own ProcChance (both 100).
-- SpellFamilyName is set on both: SpellInfo::IsAffected returns true as soon as the family name is 0,
-- so a mask without its family is never consulted. 22 is Stormbringer.
--   #3141 Pulse Conversion (707619) - "Dispelling a magic effect with Stormbreaker now heals you for
--        3% of your maximum health." Payload 504830 is effect 10 (HEAL, BasePoints 2). Stormbreaker
--        (705669) carries a single effect 38 (SPELL_EFFECT_DISPEL, misc 1 = magic), so keying the row
--        to its family flags 0/4/0 makes "Stormbreaker landed" and "dispelled with Stormbreaker" the
--        same event in every case but one: using it with nothing to dispel still heals. That single
--        over-fire is accepted rather than guessed at, since the alternative is leaving it dead.
--   #300834 Wrath of Al'Akir (300834) - "Depleting Static extends the duration of your Unshackle by
--        3 sec." Payload 300835 is effect 177 (MODIFY_AURA_DURATION, +3000ms) over 706625 Unshackle,
--        which matches the tooltip exactly. The module documents Torrential Wrath as the ability that
--        "consumes all Static", so the row keys its family flags 0/1/32. Approximation, stated rather
--        than hidden: Torrential Wrath used while Static is already empty also extends Unshackle,
--        because the Static pool is not readable from a proc event.
-- MASK CORRECTION (2026-09-22) - THIS SUPERSEDES THE MASK VALUES NAMED ABOVE: SpellFamilyMask
-- matches by ANY shared bit (flag96 operator& plus
-- operator bool in Util.h:513/557, consumed at SpellInfo.cpp:1439), so a mask set to an ability full
-- flags also matched every ability sharing any of those bits - word2 bit 32 alone is shared by about
-- thirteen Stormbringer abilities. The rows here now carry only the bits that are EXCLUSIVE to the
-- ability the clause names, so the mask selects it alone. Where an ability has no exclusive bit at
-- all (its whole flag set is shared), the mask cannot isolate it and the row must be filtered by the
-- spell list in AscensionStormbringerRunemasterTalentProcs instead.
DELETE FROM `spell_proc` WHERE `SpellId` IN (707619, 300834);
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(707619, 0, 22, 0, 4, 0, 69904, 1, 2, 0, 0, 0, 0, 0, 0, 0),
(300834, 0, 22, 0, 1, 0, 69904, 1, 2, 0, 0, 0, 0, 0, 0, 0);
