-- Legacy of the Bronzebeards (680424) makes Earthen Avatar one hundred
-- percent more effective. Both effects are native spell modifiers against
-- Earthen Avatar's family mask (0x80000 in flag B): effect 0 adds a flat
-- minus one second through SPELLMOD_COOLDOWN and effect 1 adds ninety-nine
-- percent through SPELL_AURA_ADD_PCT_MODIFIER with SPELLMOD_ALL_EFFECTS,
-- which recalculates the passive aura's amounts through CalcValue. This
-- script only registers the talent binding.
START TRANSACTION;
DELETE FROM `spell_script_names` WHERE `ScriptName` = 'aura_ascension_legacy_bronzebeards';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(680424, 'aura_ascension_legacy_bronzebeards');
COMMIT;
