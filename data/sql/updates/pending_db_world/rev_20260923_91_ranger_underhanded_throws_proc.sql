-- Underhanded Throws (804942), issue #3464: "Casting Quills now reduces the cost of subsequent Quills by
-- $573274s1% for $573274d, stacking $573274u times."
--
-- Measured before writing:
--   * 804942 carries a live SPELL_AURA_PROC_TRIGGER_SPELL (aura 42) on 573274 Underhanded Throw.
--     Spell.dbc gives the record ProcFlags 0 and the world DB holds no `spell_proc` row, so
--     SpellMgr::LoadSpellProcs skips it ("Skip if no proc flags in DBC") and Aura::GetProcEffectMask
--     returns 0 before any effect is examined - the proc has never fired.
--   * ProcChance is 100, so by this class's discriminator the aura is a marker whose event is casting
--     Quills, and Chance stays 0 so the loader keeps using that 100 (SpellMgr.cpp:2122).
--   * 573274 Underhanded Throw is native: a single aura 108 (SPELL_AURA_ADD_FLAT_MODIFIER) with base
--     points -21 on the caster over DurationIndex 32, and it is not in the EffectNULL set.
--   * Quills exists five times - 560966 and 561184 through 561187 - all SpellFamilyName 27 with
--     SpellFamilyFlags [0, 4, 131072]. Those bits ARE exclusive: across every family-27 spell in the
--     client, word1 bit 4 is held by exactly those five, and holding BOTH bits narrows it to the same
--     five (word2 bit 131072 alone is shared with 572419, which is why the mask carries both). So here
--     the mask isolates the ability and no spell list is needed - the opposite of Hailfire, where Rapid
--     Barrage carries no bit at all and had to fall back to the list.
--   * ProcFlags 69904 = the four damage-by-ability classes with no auto-attack bits, SpellTypeMask 1
--     (DAMAGE) and SpellPhaseMask 2 (HIT).
DELETE FROM `spell_proc` WHERE `SpellId` = 804942;
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(804942, 0, 27, 0, 4, 131072, 69904, 1, 2, 0, 0, 0, 0, 0, 0, 0);
