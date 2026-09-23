-- Realm of Darkness (705398) never fired.
--
-- The talent promises that Wraithblade banishes the caster and the target to the Shadowlands. Its
-- record is a SPELL_AURA_PROC_TRIGGER_SPELL whose trigger (800467) already carries the banish, but
-- the record carries ProcTypeMask 0 and the talent had no spell_proc row, so the aura was built with
-- no proc flags and nothing could reach the trigger.
--
-- The row mirrors the shape its sibling Soulrot (805196) carries, including SpellTypeMask 1 because
-- Wraithblade is a damaging ability. The Rules entry for 705398 lists Wraithblade's nine ranks, the
-- same list the 705437 entry already carries.
DELETE FROM `spell_proc` WHERE `SpellId` = 705398;
INSERT INTO `spell_proc`
  (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`,
   `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`,
   `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`)
VALUES
  (705398, 0, 0, 0, 0, 0, 69652, 1, 2, 0, 0, 0, 0, 100, 0, 0);

DELETE FROM `spell_script_names` WHERE `spell_id` = 705398
  AND `ScriptName` = 'spell_ascension_reaper_talent_proc';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(705398, 'spell_ascension_reaper_talent_proc');
