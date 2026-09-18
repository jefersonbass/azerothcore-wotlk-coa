DELETE FROM `spell_script_names` WHERE `ScriptName` = 'aura_ascension_mountain_mover';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(805643, 'aura_ascension_mountain_mover');

DELETE FROM `spell_proc` WHERE `SpellId` = 805643;
INSERT INTO `spell_proc`
(`SpellId`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `Chance`) VALUES
(805643, 1048575, 7, 2, 32767, 2, 100);
