-- Highest Order (680771): "Reduces the cooldown of Shadowsong's Mandate by
-- 25%." Binds the after-cast cooldown refund; the passive has no spell
-- family, so the native Add % Modifier slot cannot match the Mandate.
DELETE FROM `spell_script_names` WHERE `ScriptName` = 'spell_ascension_starcaller_highest_order';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(805439, 'spell_ascension_starcaller_highest_order');
