-- Barbed Quills ranks 1-2 (800077, 800080) and the bleed applier (803856), issue #3167:
-- "Damage dealt by Wild Strike, Flank and Quills will now tear into enemies, causing
-- them to bleed for 92 to 100 Physical damage over 9 sec, stacking 4 times. Damage dealt
-- by Quills now spreads your periodic effects to 5 enemies within 10 yds and extends
-- their duration by 2 sec."
--
-- Measured before writing (server-dbc/Spell.dbc):
--   * Rank 1 (800077) already has its `spell_proc` row in
--     rev_20260921_40_ranger_dead_procs.sql: three aura-42 effects on 560965 (spread),
--     561161 and 561168 (both +2000 ms duration). Its spread half was dead because
--     560965's three SPELL_EFFECT_ASCENSION_SPREAD_AURA (169) effects mapped to
--     Spell::EffectNULL; they now resolve to Spell::EffectAscensionSpreadAura
--     (SpellEffects.cpp), which copies the caster's matching aura from the struck enemy
--     to up to MaxAffectedTargets (5, the record's own value) enemies within the
--     record's 10 yd radius, carrying stacks and remaining duration.
--   * Rank 2 (800080) has a single aura-42 effect on 561021, one more effect-169 spread
--     record (TriggerSpell 500073 Serrated Shot, radius 13 = 10 yd, MaxAffectedTargets
--     5), with Spell.dbc ProcFlags 0 and no `spell_proc` row. The row below mirrors
--     rank 1's exactly: family 27 with SpellFamilyMask1 4, carried by the five Quills
--     records (560966, 561184-561187) and nothing else in family 27, ProcFlags 256 =
--     DONE_SPELL_RANGED_DMG_CLASS for the DmgClass 3 Quills shots, type 1 / phase 2.
--   * Neither rank applies the bleed itself: no effect of 800077 or 800080 names 801472.
--     The bleed lives on Barbed Quills 803856, a single aura-42 effect directly on 801472
--     (native 9-second periodic, bleed mechanic) with Spell.dbc ProcFlags 0 and no row.
--     Its event is the tooltip's first paragraph, so the row scopes all three named
--     abilities in one mask: SpellFamilyMask1 131332 = 4 (Quills) | 256 (Wild Strike,
--     every rank carries word-1 bit 0x100 and the off-hand halves do not, so one strike
--     procs once) | 131072 (Flank, all ten records carry word-1 bit 0x20000, the same
--     ten AscensionClassMechanics.cpp IsRangerFlank recognises). ProcFlags 272 =
--     DONE_SPELL_MELEE_DMG_CLASS (Flank and Wild Strike are DmgClass 2) |
--     DONE_SPELL_RANGED_DMG_CLASS (Quills is DmgClass 3); type 1, phase 2, no crit
--     requirement, Chance 0 so each record's own ProcChance 100 applies.
DELETE FROM `spell_proc` WHERE `SpellId` IN (800080, 803856);
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(800080, 0, 27, 0, 4, 0, 256, 1, 2, 0, 0, 0, 0, 0, 0, 0),
(803856, 0, 27, 0, 131332, 0, 272, 1, 2, 0, 0, 0, 0, 0, 0, 0);
