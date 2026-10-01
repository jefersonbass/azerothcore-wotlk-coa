-- Ancestral Evolution 806229 (Barbarian raid talent): "all party and raid members reflect 20% of all damage taken
-- for 20 sec". The raid-wide carrier ships effect 0 as a DUMMY aura (BasePoints 19 -> amount 20) with ProcFlags 0,
-- so nothing ever fires. The load-time contract repoints the aura at PROC_TRIGGER_SPELL_WITH_VALUE and arms
-- damage-taken procs; the aura script reflects the share back at the attacker. Register the AuraScript binding here.
DELETE FROM `spell_script_names` WHERE `spell_id` = 806229 AND `ScriptName` = 'aura_ascension_ancestral_evolution';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(806229, 'aura_ascension_ancestral_evolution');
