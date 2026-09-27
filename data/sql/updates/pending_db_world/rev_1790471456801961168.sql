-- Death From Above (#1133): bind the ability script so the Draconic Aspect gate runs.
-- 520402 has no spell_script_names row, so spell_ascension_pyromancer_ability (with
-- its Check) never executed for it; without the row the cast below also never ran.
START TRANSACTION;
DELETE FROM `spell_script_names` WHERE `spell_id` = 520402 AND `ScriptName` = 'spell_ascension_pyromancer_ability';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (520402, 'spell_ascension_pyromancer_ability');
COMMIT;
