-- Electrified Waters (573436, issue #929): "Your direct critical strikes against an enemy affected by
-- Drown spawn an Electrified Water Elemental." The record authors aura 42 with TriggerSpell 573437
-- (effect 64 summoning creature 150), but Spell.dbc gives it ProcFlags 0, so SpellMgr::LoadSpellProcs
-- never generated a proc entry and the proc never fired.
-- The critical half is expressible in the row: ProcFlags 69972 (the four direct damage spell classes
-- plus melee and ranged auto attacks) with HitMask 2 (PROC_HIT_CRITICAL) and SpellPhaseMask 2. The
-- clause names no ability, so the mask and family stay 0 - the target condition is the only filter.
-- The target half is not expressible in `spell_proc`, so a dedicated AuraScript alongside the Rules
-- table (aura_stormbringer_electrified_waters, bound below) requires the struck unit to carry Drown
-- (572760 or its second rank 572761). Per the team rule, a one-off condition is a script beside the
-- table, never a new field on the shared Rule struct.
-- Chance stays 0, deferring to the record's own ProcChance (100).
DELETE FROM `spell_proc` WHERE `SpellId` = 573436;
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(573436, 0, 0, 0, 0, 0, 69972, 1, 2, 2, 0, 0, 0, 0, 0, 0);

DELETE FROM `spell_script_names` WHERE `spell_id` = 573436
  AND `ScriptName` = 'aura_stormbringer_electrified_waters';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(573436, 'aura_stormbringer_electrified_waters');
