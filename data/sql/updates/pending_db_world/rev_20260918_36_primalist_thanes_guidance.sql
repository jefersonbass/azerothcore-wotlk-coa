DELETE FROM `spell_script_names` WHERE `ScriptName` = 'aura_ascension_thanes_guidance';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(680410, 'aura_ascension_thanes_guidance');

DELETE FROM `spell_proc` WHERE `SpellId` = 680410;
INSERT INTO `spell_proc`
(`SpellId`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `Chance`) VALUES
(680410, 4, 0, 0, 0, 0, 100);
