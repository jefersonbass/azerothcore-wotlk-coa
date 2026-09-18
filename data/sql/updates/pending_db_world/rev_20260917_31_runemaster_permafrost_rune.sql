-- Issue #455: Runemaster "Permafrost Rune" (804060).
-- - spell_ascension_runemaster_permafrost_rune: enforce "Requires Frozen Target"
--   on cast and the Runeshroud 80% cooldown discount.
-- - aura_ascension_runemaster_permafrost_rune: clamp duration to 8 sec vs players.
-- "Damage taken will end the effect" is handled by the UnitScript registered in
-- AscensionRunemasterGlyphs.cpp (no DB binding needed).
DELETE FROM `spell_script_names` WHERE `spell_id` = 804060 AND `ScriptName` IN
('spell_ascension_runemaster_permafrost_rune', 'aura_ascension_runemaster_permafrost_rune');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(804060, 'spell_ascension_runemaster_permafrost_rune'),
(804060, 'aura_ascension_runemaster_permafrost_rune');
