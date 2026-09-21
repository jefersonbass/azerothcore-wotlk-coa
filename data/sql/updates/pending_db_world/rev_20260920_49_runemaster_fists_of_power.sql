-- CoA Runemaster Fists of Power (805796, issue 663): while the buff is active, melee
-- auto attacks grant Earthen Fists (806982, 10 sec, 5 stacks). Spell.dbc already
-- carries the proc aura (42) and its trigger, but no spell_proc row existed, so the
-- aura was never prepared and the proc never fired.
-- ProcFlags 0x4 = done melee auto attack, matching the base-game auto-attack procs.
-- Chance 0 defers to the Spell.dbc ProcChance (100).
START TRANSACTION;
DELETE FROM `spell_proc` WHERE `SpellId` = 805796;
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(805796, 0, 0, 0, 0, 0, 0x4, 0, 0, 0, 0, 0, 0, 0, 0, 0);
COMMIT;
