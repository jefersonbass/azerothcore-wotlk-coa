-- Infinite Power (92118): "Casting damaging spells now reduces your damaging spell
-- cooldowns by $528312s1% of their remaining cooldown." The talent procs helper 503946,
-- whose three effect-192 slots trim the remaining cooldown of 801291, 801292 and 806335
-- by $528312s1%. Spell.dbc gives 92118 ProcFlags 0 and no spell_proc row existed, so the
-- helper never fired. Proc on damaging spell hits: melee class (0x10), magic
-- negative (0x10000) and periodic damage (0x40000), 100% chance; the helper names
-- the three cooldowns explicitly, so no family mask is needed.
DELETE FROM `spell_proc` WHERE `SpellId` = 92118;
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`,
`SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`,
`DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(92118, 0, 0, 0, 0, 0, 327696, 0, 1, 0, 0, 0, 0, 100, 0, 0);
