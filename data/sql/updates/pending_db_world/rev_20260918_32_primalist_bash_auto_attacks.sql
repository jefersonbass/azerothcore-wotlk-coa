DELETE FROM `spell_script_names` WHERE `ScriptName` = 'aura_ascension_bash';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(680964, 'aura_ascension_bash');

DELETE FROM `spell_proc` WHERE `SpellId` = 680964;
INSERT INTO `spell_proc`
(`SpellId`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `Chance`) VALUES
(680964, 4, 0, 0, 0, 0, 30);
