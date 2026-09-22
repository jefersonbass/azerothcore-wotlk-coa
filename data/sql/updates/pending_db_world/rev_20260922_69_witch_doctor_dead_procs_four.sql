-- Closing Witch Doctor batch, same shape as rev_20260922_66/67/68: an aura-42 passive with Spell.dbc
-- ProcFlags 0 and no `spell_proc` row, so the clause never fired while the payload stays authored (the
-- aura-42 handler casts the record's own TriggerSpell). Both payloads are family 19 like the talent.
-- Both clauses are "after casting X", so SpellPhaseMask 1 = PROC_SPELL_PHASE_CAST with SpellTypeMask 7 (all
-- types) and ProcFlags 72464 = 69904 (the four direct damage classes) | 2560 (the beneficial pair) - a strike
-- or a potion toss is a positive spell, so its cast event carries the positive flags rather than the negative
-- ones.
-- My Gods Are Forever (802267): "Strike of the Gods now increases the effectiveness of your next Appeasement"
-- - Strike of the Gods (302531, 501160, 501161) carries zero FamilyFlags, so the row stays family 0 with no
-- mask and aura_ascension_spell_list_talent_proc filters against AscensionSpellListTalentProcs.h.
-- Snake Oil (806271): "Potion Toss now increases the healing of your Loa's Brew" - Potion Toss
-- (573430-573435, 801653, 801661) is isolated by flags[1] 0x80800 (8 holders, all Potion Toss). The record
-- also lacks SPELL_ATTR0_PASSIVE; the class contract re-marks it so the apply passes install the aura.
DELETE FROM `spell_proc` WHERE `SpellId` IN (802267, 806271);
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(802267, 0, 0, 0, 0, 0, 72464, 7, 1, 0, 0, 0, 0, 0, 0, 0),
(806271, 0, 19, 0, 526336, 0, 72464, 7, 1, 0, 0, 0, 0, 0, 0, 0);

DELETE FROM `spell_script_names` WHERE `ScriptName` = 'spell_ascension_spell_list_talent_proc' AND `spell_id` = 802267;
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(802267, 'spell_ascension_spell_list_talent_proc');
