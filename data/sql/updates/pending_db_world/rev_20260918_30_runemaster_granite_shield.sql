-- Granite Shield (806996): "While Runic Tattoos: Earth is active, you now periodically gain
-- Granite Shield every 20 sec." Binds the tick-gate aura script; the dead DBC trigger is
-- repointed at the real absorb (520822) in code via OnLoadSpellCustomAttr.
DELETE FROM `spell_script_names` WHERE `ScriptName` = 'aura_runemaster_granite_shield';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(806996, 'aura_runemaster_granite_shield');
