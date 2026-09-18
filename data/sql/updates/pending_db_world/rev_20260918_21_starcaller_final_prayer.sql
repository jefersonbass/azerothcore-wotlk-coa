-- Issue #863: Starcaller "Final Prayer" (704745) — "Your Prayer of Elune now
-- dispels 1 additional effect." The Prayer's Dispel effect charge count is
-- fixed in the DBC; the extra dispel lives in
-- AscensionStarcaller.cpp and needs this binding on the Prayer chain root.
DELETE FROM `spell_script_names` WHERE `spell_id` = -801987 AND `ScriptName` = 'spell_ascension_starcaller_prayer_of_elune';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(-801987, 'spell_ascension_starcaller_prayer_of_elune');
