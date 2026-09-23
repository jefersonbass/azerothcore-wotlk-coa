-- Rancid Air (#2374): the "+5 Runic Power" half of "Putrefy now generates an additional 5 Runic Power and
-- the cast time is reduced by 10%".
--
-- The cast-time half travels the native spellmod path, which the routing committed in 8fc77419f restored
-- for this talent. The resource half has no SpellModOp - the record's ops are SPELLMOD_SPEED (12) and
-- SPELLMOD_CASTING_TIME (10) - so spell_ascension_necromancer_ability grants it instead.
--
-- Only the player-cast records are bound: casting Putrefy 561125 triggers 804559, which already energizes
-- +150 Runic Power, so binding the triggered spell too would pay the bonus twice. 9666680 is the other
-- Putrefy record a player can cast.
--
-- The amount is the tooltip's own $/10;S1 over base points 49, and the original reference data renders that
-- record as "an additional 5 Runic Power".
DELETE FROM `spell_script_names` WHERE `spell_id` IN (561125, 9666680) AND `ScriptName` = 'spell_ascension_necromancer_ability';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(561125, 'spell_ascension_necromancer_ability'),
(9666680, 'spell_ascension_necromancer_ability');
