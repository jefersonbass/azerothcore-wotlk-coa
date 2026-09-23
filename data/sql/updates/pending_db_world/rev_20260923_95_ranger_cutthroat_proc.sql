-- Cutthroat (705065), issue #2502: "Ambushes dealt by Flank increase the damage the target takes from you
-- by $573280s1% for $573280d, stacking up to $573280u times." (The talent's other half, increased damage
-- against bleeding enemies, is a native aura 303 and already works.)
--
-- Measured before writing:
--   * 705065 carries a live SPELL_AURA_PROC_TRIGGER_SPELL (aura 42) on 573280 Cutthroat. Spell.dbc gives
--     the record ProcFlags 0 and the world DB holds no `spell_proc` row, so SpellMgr::LoadSpellProcs
--     skips it ("Skip if no proc flags in DBC") and Aura::GetProcEffectMask returns 0 before any effect
--     is examined - the proc has never fired.
--   * ProcChance is 100, so the aura is a marker whose event is Flank, and Chance stays 0 so the loader
--     keeps using it (SpellMgr.cpp:2122).
--   * 573280 is a single aura 271 on the target (target 6) with base points 2, and it is not in the
--     EffectNULL set, so the payload delivers.
--   * Flank exists ten times - 582530, 582531 and 804940, 805082 through 805088 - all SpellFamilyName 27
--     with SpellFamilyFlags [0, 131072, 0]. Counting every family-27 spell holding word1 bit 131072
--     returns exactly those ten and nothing else, so the bit isolates the ability and the mask carries
--     it. No Rules entry and no aura script are needed: the aura-42 default action casts the trigger.
--     Same shape as Underhanded Throws and Whipping Pressure.
--   * ProcFlags 69904 = the four damage-by-ability classes with no auto-attack bits, SpellTypeMask 1
--     (DAMAGE) and SpellPhaseMask 2 (HIT).
DELETE FROM `spell_proc` WHERE `SpellId` = 705065;
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(705065, 0, 27, 0, 131072, 0, 69904, 1, 2, 0, 0, 0, 0, 0, 0, 0);
