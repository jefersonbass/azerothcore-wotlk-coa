DELETE FROM `spell_script_names` WHERE `ScriptName` IN ('aura_ascension_terrasmash', 'aura_ascension_resources_of_the_earth');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(706220, 'aura_ascension_terrasmash'),
(560548, 'aura_ascension_resources_of_the_earth');

DELETE FROM `spell_proc` WHERE `SpellId` IN (706220, 560548);
INSERT INTO `spell_proc`
(`SpellId`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `Chance`) VALUES
(706220, 8388608, 0, 0, 0, 0, 30),
(560548, 15036476, 0, 0, 2, 0, 100);
