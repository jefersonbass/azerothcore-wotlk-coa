-- Adaptation: Brigand (800089), issue #2080: "Critical strikes with Wild Strike and Flank have a $h%
-- chance to leave the target Roughed Up, ..."
--
-- Measured before writing:
--   * 800089 carries a live SPELL_AURA_PROC_TRIGGER_SPELL (aura 42) on 800082 Roughed Up. Spell.dbc
--     gives the record ProcFlags 0 and the world DB holds no `spell_proc` row, so
--     SpellMgr::LoadSpellProcs skips it ("Skip if no proc flags in DBC") and Aura::GetProcEffectMask
--     returns 0 before any effect is examined - the proc has never fired.
--   * ProcChance is 50, so this is a real chance proc rather than a marker, and Chance stays 0 so the
--     loader falls back to that value (SpellMgr.cpp:2122).
--   * 800082 is native: auras 22, 189 and 271 on the target, none of them in the EffectNULL set.
--   * The tooltip names TWO abilities, so the mask must cover both. Counting every family-27 spell:
--     word1 bit 256 is held by 14 spells, all named Wild Strike, and word1 bit 131072 by 10, all named
--     Flank; no spell holds both. Each bit is therefore exclusive to its ability and the mask carries
--     the union (256 | 131072 = 131328), which matches exactly those 24 and nothing else - the mask
--     matches on any shared bit.
--   * Deliberately not covered: 561352, also named Wild Strike but with SpellFamilyFlags [1073741824,
--     0, 0]. It is the off-hand sub-spell that the main Wild Strike (501724) triggers through its
--     effect 64 on 560962, not an ability the player casts, so the cast the tooltip keys on is the main
--     one. Noted here because the count above surfaces it.
--   * ProcFlags 69904 = the four damage-by-ability classes with no auto-attack bits, SpellTypeMask 1
--     (DAMAGE) and SpellPhaseMask 2 (HIT).
DELETE FROM `spell_proc` WHERE `SpellId` = 800089;
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(800089, 0, 27, 0, 131328, 0, 69904, 1, 2, 0, 0, 0, 0, 0, 0, 0);
