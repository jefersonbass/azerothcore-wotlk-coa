-- Lost Embrace (705402) never fired.
--
-- The talent promises that Spectre Stride slows enemies by 60% for 6 seconds. Its record is a
-- SPELL_AURA_PROC_TRIGGER_SPELL whose trigger (800422) already applies the 60% slow - an
-- APPLY_AURA at -61 base points - but the record carries ProcTypeMask 0 and the talent had no
-- spell_proc row, so the aura was built with no proc flags and nothing could reach the trigger.
--
-- The row mirrors the shape its sibling Soulrot (805196) carries. SpellTypeMask 7 keeps every spell
-- type eligible and the Rules entry for 705402 narrows the event to Spectre Stride's ranks.
DELETE FROM `spell_proc` WHERE `SpellId` = 705402;
INSERT INTO `spell_proc`
  (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`,
   `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`,
   `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`)
VALUES
  (705402, 0, 0, 0, 0, 0, 69652, 7, 2, 0, 0, 0, 0, 100, 0, 0);

DELETE FROM `spell_script_names` WHERE `spell_id` = 705402
  AND `ScriptName` = 'spell_ascension_reaper_talent_proc';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(705402, 'spell_ascension_reaper_talent_proc');
