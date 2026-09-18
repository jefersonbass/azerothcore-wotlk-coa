-- Protective Warding (800756): "Critical damage taken reduces the cooldown of
-- Rune of Guarding by 10%." Binds the crit-filter proc script; the missing taken
-- proc flags are armed in code via OnLoadSpellCustomAttr.
DELETE FROM `spell_script_names` WHERE `ScriptName` = 'aura_runemaster_protective_warding';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(800756, 'aura_runemaster_protective_warding');
