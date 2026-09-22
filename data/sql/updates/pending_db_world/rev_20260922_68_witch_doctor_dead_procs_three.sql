-- Third Witch Doctor batch, same shape as rev_20260922_66/67: an aura-42 passive with Spell.dbc ProcFlags 0
-- and no `spell_proc` row, so the clause never fired while the payload stays authored (the aura-42 handler
-- casts the record's own TriggerSpell). Both payloads are family 19 like the talent.
-- Hexblade (802093): "Your melee auto attacks have a 27% chance to deal additional Shadow damage" - ProcFlags
-- 4 = PROC_FLAG_DONE_MELEE_AUTO_ATTACK, outside the spell and phase masks; Chance 0 defers to the record's
-- own ProcChance = 27.
-- Master Brewer (802218): "Healing done with Spirit in a Bottle or Loa's Brew has a 15% chance" - ProcFlags
-- 2560 (the beneficial spell pair, which is what a heal raises) with SpellTypeMask 2 (heal); Chance 0 defers
-- to the record's ProcChance = 15. The mask cannot name both abilities: Loa's Brew's flags[1] 0x20000 is
-- exclusive, but Spirit in a Bottle's flags[2] 0x1 is also carried by Witch Doctor Saurid/Raptor/Diemetradon
-- Scaling (805992-805994), so the row stays family 0 with no mask and
-- aura_ascension_spell_list_talent_proc filters against AscensionSpellListTalentProcs.h (23 ids). The record
-- also lacks SPELL_ATTR0_PASSIVE; the class contract re-marks it so the apply passes install the aura.
DELETE FROM `spell_proc` WHERE `SpellId` IN (802093, 802218);
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(802093, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0),
(802218, 0, 0, 0, 0, 0, 2560, 2, 2, 0, 0, 0, 0, 0, 0, 0);

DELETE FROM `spell_script_names` WHERE `ScriptName` = 'spell_ascension_spell_list_talent_proc' AND `spell_id` = 802218;
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(802218, 'spell_ascension_spell_list_talent_proc');
