-- CoA Runic Obliteration (705560): "Using Elemental Burst now has a $h% chance to transform your next
-- Primordial Blast into Runic Obliteration". The talent's aura proc never fired.
--
-- Measured before writing:
--   * 705560 is SPELL_AURA_PROC_TRIGGER_SPELL (aura 42) with EffectTriggerSpell 805742 and
--     ProcChance 20. A chance below 100 means the aura IS the mechanism, not a marker - the marker
--     case in this class always carries ProcChance 100.
--   * Its Spell.dbc ProcFlags are 0 and the world database had no spell_proc row for it, so the proc
--     carried no ProcFlags at all. Same shape as Ice Engraving (653266) and Command: Undead.
--   * 805742 is the Runic Obliteration buff the talent is supposed to grant; it exists in Spell.dbc.
--   * The class already implements "procs when the player casts one of these spells" through
--     AscensionStormbringerRunemasterTalentProcs: a Rules table entry (talent -> triggering spells)
--     plus this same spell_proc row shape. 520138 is the closest sibling - same family, same Elemental
--     Burst spell list, same flags - so this mirrors it field for field.
--   * SpellFamilyMask stays 0 because the row is filtered by the spell list in the Rules table, which
--     is the documented convention for abilities with no exclusive family-mask bit.
--
-- The Rules entry for 705560 is added in AscensionStormbringerRunemasterTalentProcs.h with the
-- Elemental Burst spell list, so the proc only fires from Elemental Burst as the tooltip says.
DELETE FROM `spell_proc` WHERE `SpellId` = 705560;
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(705560, 0, 38, 0, 0, 0, 69904, 1, 2, 0, 0, 0, 0, 0, 0, 0);

DELETE FROM `spell_script_names` WHERE `spell_id` = 705560 AND `ScriptName` = 'spell_ascension_stormbringer_runemaster_talent_proc';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(705560, 'spell_ascension_stormbringer_runemaster_talent_proc');
