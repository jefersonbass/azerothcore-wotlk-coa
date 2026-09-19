-- The Vast Infinite (#1106): bind the share-tracking aura script to the raid aura.
START TRANSACTION;
DELETE FROM `spell_script_names` WHERE `spell_id` = 706083 AND `ScriptName` = 'aura_vast_infinite';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (706083, 'aura_vast_infinite');
COMMIT;
