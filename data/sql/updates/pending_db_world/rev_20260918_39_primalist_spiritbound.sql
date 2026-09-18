DELETE FROM `spell_script_names` WHERE `ScriptName` = 'aura_ascension_spiritbound';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(681364, 'aura_ascension_spiritbound');

DELETE FROM `spell_proc` WHERE `SpellId` = 681364;
INSERT INTO `spell_proc`
(`SpellId`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `Chance`) VALUES
(681364, 1048575, 7, 2, 32767, 2, 100);
