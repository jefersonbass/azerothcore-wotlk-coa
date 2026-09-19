-- Earth Destroyer (560508) shortens Mountain Fury's cooldown by twenty seconds
-- and extends its duration by four seconds. The plus four second duration is
-- native through the authored Add Flat Modifier (effect 0,
-- SPELLMOD_DURATION with the Mountain Fury class mask). The local Spell.dbc
-- carries no cooldown for Mountain Fury (806185), so the official
-- forty-five second cooldown is set by the Mountain Fury script on cast and
-- shortened to twenty-five seconds while this aura is held; this script only
-- registers the talent binding.
START TRANSACTION;
DELETE FROM `spell_script_names` WHERE `ScriptName` IN ('aura_ascension_earth_destroyer');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(560508, 'aura_ascension_earth_destroyer');
COMMIT;
