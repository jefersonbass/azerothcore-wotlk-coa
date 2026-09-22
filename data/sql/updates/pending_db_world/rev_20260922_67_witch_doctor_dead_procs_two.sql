-- Second Witch Doctor batch, same shape as rev_20260922_66: an aura-42 passive with Spell.dbc ProcFlags 0 and
-- no `spell_proc` row, so the clause never fired while the payload stays authored (the aura-42 handler casts
-- the record's own TriggerSpell). Every payload is family 19 like the talent.
-- Masks were measured with mask_scan.py (IsAffected matches on ANY shared bit):
--   Malefic Arrow (680908-680910, 801674, ...) is isolated by flags[1] 0x80000000 alone - its 0x8000 bit is
--   shared with Malefic Wrath, so the row uses only the exclusive bit.
--   Shadow Puppets (500015, 501080-501088) is isolated by flags[0] 0x8 alone.
--   Voodoo Puddle (803200) carries zero FamilyFlags, so Rogue Spirits stays family 0 with no mask and
--   aura_ascension_spell_list_talent_proc filters the event spell against AscensionSpellListTalentProcs.h.
-- Trapped Spirits (705897): "Damage dealt by Malefic Arrow" - ProcFlags 69904 (the four direct damage spell
-- classes) with SpellTypeMask 1 and SpellPhaseMask 2 (hit).
-- Rogue Spirits (705915): "Damage dealt by Voodoo Puddle" - same event shape, both of its effects
-- (524700 movement slow, 524849 Serpent Beam taken-damage) ride the one proc.
-- Agony (705919): "Damage dealt by Shadow Puppets" - same event shape. The record also lacks
-- SPELL_ATTR0_PASSIVE; the class contract re-marks it so the apply passes install the aura.
DELETE FROM `spell_proc` WHERE `SpellId` IN (705897, 705915, 705919);
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(705897, 0, 19, 0, 2147483648, 0, 69904, 1, 2, 0, 0, 0, 0, 0, 0, 0),
(705915, 0, 0, 0, 0, 0, 69904, 1, 2, 0, 0, 0, 0, 0, 0, 0),
(705919, 0, 19, 8, 0, 0, 69904, 1, 2, 0, 0, 0, 0, 0, 0, 0);

DELETE FROM `spell_script_names` WHERE `ScriptName` = 'spell_ascension_spell_list_talent_proc' AND `spell_id` = 705915;
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(705915, 'spell_ascension_spell_list_talent_proc');
