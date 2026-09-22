-- Pulse Conversion (707619, issue #3141): "Dispelling a magic effect with Stormbreaker now heals you for
-- 3% of your maximum health." The record authors aura 42 with TriggerSpell 504830 (effect 10 HEAL,
-- BasePoints 2), but Spell.dbc gives it ProcFlags 0, so the proc never fired.
-- The clause is not expressible as a proc event: "dispelling" is not a hit, and the heal must not fire
-- when the target had nothing to dispel. It is handled in code instead, by
-- spell_ascension_stormbringer_pulse_conversion (AscensionStormbringerTalents.cpp), which runs on
-- Stormbreaker's own SPELL_EFFECT_DISPEL and heals only when GetDispellableAuraList finds a magic effect
-- to remove. Binding that script is the only SQL this talent needs - there is deliberately no
-- `spell_proc` row, which would double the heal through the record's own aura 42.
DELETE FROM `spell_script_names` WHERE `spell_id` = 705669
  AND `ScriptName` = 'spell_ascension_stormbringer_pulse_conversion';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(705669, 'spell_ascension_stormbringer_pulse_conversion');
