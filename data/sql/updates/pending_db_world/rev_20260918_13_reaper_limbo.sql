-- Issue #472: Reaper "Limbo" (800845) — casting it must apply the mortal shell
-- aura (805872, School Immunity + client visual). Heal %, Energize % and the
-- usable-while-CC flags come straight from the Spell.dbc.
DELETE FROM `spell_script_names` WHERE `spell_id` = 800845 AND `ScriptName` = 'spell_ascension_reaper_limbo';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(800845, 'spell_ascension_reaper_limbo');
