-- Just A Sip (#1143): drinking Tonics restores 5% health and mana.
-- Bind the cast hook to every Tonic cast spell; negative ids cover all ranks.
START TRANSACTION;
DELETE FROM `spell_script_names` WHERE `ScriptName` = 'spell_ascension_witch_hunter_just_a_sip' AND `spell_id` IN (-680491, 802276, 802277, 802278, -802279, 802826);
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(-680491, 'spell_ascension_witch_hunter_just_a_sip'),
(802276, 'spell_ascension_witch_hunter_just_a_sip'),
(802277, 'spell_ascension_witch_hunter_just_a_sip'),
(802278, 'spell_ascension_witch_hunter_just_a_sip'),
(-802279, 'spell_ascension_witch_hunter_just_a_sip'),
(802826, 'spell_ascension_witch_hunter_just_a_sip');
COMMIT;
