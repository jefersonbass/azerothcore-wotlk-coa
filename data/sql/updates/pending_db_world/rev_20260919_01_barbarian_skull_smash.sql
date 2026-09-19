-- Skull Smash (#1144): bind the lifecycle aura script so the authored 8s player cap applies.
START TRANSACTION;
DELETE FROM `spell_script_names` WHERE `spell_id` = 560532 AND `ScriptName` = 'aura_ascension_barbarian_lifecycle';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (560532, 'aura_ascension_barbarian_lifecycle');
COMMIT;
