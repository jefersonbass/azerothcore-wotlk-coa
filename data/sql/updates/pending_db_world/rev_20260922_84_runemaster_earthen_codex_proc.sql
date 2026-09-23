-- CoA Earthen Codex (707460): "When you activate a Palm Sigil, you are now guaranteed to critically
-- strike for $707730s2% increased critical damage for $707730d or your next 3 attacks". The talent's
-- proc never fired, so the buff was never granted.
--
-- Measured before writing:
--   * 707460 is SPELL_AURA_PROC_TRIGGER_SPELL (aura 42) with EffectTriggerSpell 707730 and ProcChance
--     100, so by this class's discriminator it is a marker whose event is the Palm Sigil activation.
--   * Its Spell.dbc ProcFlags are 0 and the world database had no spell_proc row, so the trigger never
--     fired.
--   * 707730 is the buff itself: two effects, auras 290 and 163, both targeting the caster (target 1),
--     so the default aura-42 action casts it on the right unit and no aura script is needed.
--   * "Palm Sigil" is a family of 23 spells and the tooltip names none of them, so the proc has to
--     cover all 23. This is the same event as Sigilist (705586), so it takes the same shape: one Rules
--     entry carrying the 23 ids, rather than 23 registrations.
--   * SpellPhaseMask is 1 (CAST) because the tooltip keys on activating the sigil, matching Sigilist and
--     the 16 existing rows that pair ProcFlags 69904 with phase 1.
--
-- The Rules entry for 707460 is added in AscensionStormbringerRunemasterTalentProcs.h. The existing
-- aura_ascension_arcane_palm_sigil handler is untouched: it fires on direct magic damage and detonates
-- the sigil, a different moment from the activation this talent names.
DELETE FROM `spell_proc` WHERE `SpellId` = 707460;
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(707460, 0, 38, 0, 0, 0, 69904, 1, 1, 0, 0, 0, 0, 0, 0, 0);

DELETE FROM `spell_script_names` WHERE `spell_id` = 707460 AND `ScriptName` = 'spell_ascension_stormbringer_runemaster_talent_proc';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(707460, 'spell_ascension_stormbringer_runemaster_talent_proc');
