-- Withering Venom (#3190): wire the cast and its stack growth.
-- 800886 has no spell_script_names row, so spell_ascension_venomancer_ability
-- never ran for it; the bind below routes the cast through the script that
-- consumes a Lethal Toxin stack and applies the first stack helper 707084.
-- The lifecycle bind on 707084 already exists (rev_20260910_14); the per-tick
-- step to the next helper runs in that script.
START TRANSACTION;
DELETE FROM `spell_script_names` WHERE `spell_id` = 800886 AND `ScriptName` = 'spell_ascension_venomancer_ability';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (800886, 'spell_ascension_venomancer_ability');
COMMIT;
