-- CoA Glyph of Disappearance (560037): "Your Phase Out can now be used 1 additional time within
-- $561059d before incurring a cooldown". The talent's proc never fired, so the extra use never came.
--
-- Measured before writing:
--   * 560037 is SPELL_AURA_PROC_TRIGGER_SPELL (aura 42) with EffectTriggerSpell 561059 and ProcChance
--     100, so by this class's discriminator it is a marker whose event is casting Phase Out.
--   * Its Spell.dbc ProcFlags are 0 and the world database had no spell_proc row, so the trigger never
--     fired.
--   * 561059 carries SPELL_EFFECT_ASCENSION_RESET_COOLDOWN (effect 195) with EffectMiscValue 500671,
--     which is exactly Phase Out, and that effect is implemented in the core
--     (SpellEffects.cpp:541, EffectAscensionResetCooldown uses effect.MiscValue). Both of its effects
--     target the caster, so the default aura-42 action delivers it correctly and no aura script is
--     needed.
--   * Phase Out itself is already handled by the module (AscensionRunemasterRiftClones.cpp:20,
--     SPELL_PHASE_OUT = 500671), so the talent plugs into an ability the class already models.
--   * SpellPhaseMask is 1 (CAST) because the tooltip keys on USING Phase Out, matching Sigilist and
--     Earthen Codex and the 16 existing rows that pair ProcFlags 69904 with phase 1.
--
-- The Rules entry for 560037 is added in AscensionStormbringerRunemasterTalentProcs.h.
DELETE FROM `spell_proc` WHERE `SpellId` = 560037;
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(560037, 0, 38, 0, 0, 0, 69904, 1, 1, 0, 0, 0, 0, 0, 0, 0);

DELETE FROM `spell_script_names` WHERE `spell_id` = 560037 AND `ScriptName` = 'spell_ascension_stormbringer_runemaster_talent_proc';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(560037, 'spell_ascension_stormbringer_runemaster_talent_proc');
