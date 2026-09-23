-- Transfer Life (#3477): the channel 805143 is a DUMMY with no effects of its own - its only effect is
-- type 3 - and nothing applied the partner aura, so the channel healed nothing. The aura it should apply,
-- 801530, already carries everything the tooltip promises: EffectAmplitude[1] 1000 ms, EffectRadiusIndex[0]
-- 9 and a trigger on 801545, the heal formula ("$801545s1% of their maximum health plus $SP*.25"). The
-- handler in AscensionNecromancerAbilities.cpp casts that aura on the caster's Raised minions inside the
-- channel's own radius, so nothing is invented here.
DELETE FROM `spell_script_names` WHERE `spell_id` = 805143 AND `ScriptName` = 'spell_ascension_necromancer_ability';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(805143, 'spell_ascension_necromancer_ability');
