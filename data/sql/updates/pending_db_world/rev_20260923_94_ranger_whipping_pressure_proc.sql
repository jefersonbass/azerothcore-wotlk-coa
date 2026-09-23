-- Whipping Pressure (705100), issue #2523: "Whipvine Arrow used with 5 stacks of Advantage now increases
-- the spell and ranged haste of party members within $520625a1 yds by ${$AGI*0.025}% for $520625d."
--
-- Measured before writing:
--   * 705100 carries a live SPELL_AURA_PROC_TRIGGER_SPELL (aura 42) on 520625 Whipping Pressure.
--     Spell.dbc gives the record ProcFlags 0 and the world DB holds no `spell_proc` row, so
--     SpellMgr::LoadSpellProcs skips it ("Skip if no proc flags in DBC") and Aura::GetProcEffectMask
--     returns 0 before any effect is examined - the proc has never fired.
--   * ProcChance is 100, so the aura is a marker whose event is using Whipvine Arrow, and Chance stays 0
--     so the loader keeps using it (SpellMgr.cpp:2122).
--   * 520625 is a single SPELL_EFFECT 64 with target 22, which matches the tooltip's "party members
--     within range"; effect 64 is implemented and not in the EffectNULL set.
--   * Whipvine Arrow exists once, 806342, with SpellFamilyName 27 and SpellFamilyFlags [65536, 0, 0].
--     Counting every family-27 spell holding word0 bit 65536 returns exactly that one, so the bit
--     isolates the ability and the mask carries it - the row needs no Rules entry and no aura script,
--     since the aura-42 default action casts the trigger. This is the same shape as Underhanded Throws
--     and the opposite of Hailfire, Vile and Excruciating Poison, where the ability had no bit or a
--     shared one and the row had to fall back to the spell list.
--   * ProcFlags 69904 = the four damage-by-ability classes with no auto-attack bits, SpellTypeMask 1
--     (DAMAGE) and SpellPhaseMask 2 (HIT).
DELETE FROM `spell_proc` WHERE `SpellId` = 705100;
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(705100, 0, 27, 65536, 0, 0, 69904, 1, 2, 0, 0, 0, 0, 0, 0, 0);
