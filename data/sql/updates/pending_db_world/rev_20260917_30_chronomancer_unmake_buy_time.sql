-- Issue #449: Chronomancer "Buy Time" (520188) — Casting Unmake on a stasis
-- target must remove the Buy Time effect. Unmake (804418, chain root) carries a
-- Dummy slot for this interaction; bind it to the handler in
-- AscensionChronomancerTime.cpp. Negative spell_id attaches the script to every
-- rank of the chain.
DELETE FROM `spell_script_names` WHERE `spell_id` = -804418 AND `ScriptName` = 'spell_ascension_chronomancer_unmake';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(-804418, 'spell_ascension_chronomancer_unmake');
