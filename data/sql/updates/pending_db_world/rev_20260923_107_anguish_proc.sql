-- Anguish (705430) never fired.
--
-- The talent promises that Withering Touch silences enemies for 2 seconds on top of the extra second
-- it adds to the duration. Its record is a SPELL_AURA_PROC_TRIGGER_SPELL whose trigger (707378)
-- already applies the 2 second silence - an APPLY_AURA at +1999 base points - but the record carries
-- ProcTypeMask 0 and the talent had no spell_proc row, so the aura was built with no proc flags and
-- nothing could reach the trigger.
--
-- The row mirrors the shape its sibling Soulrot (805196) carries. SpellTypeMask 7 keeps every spell
-- type eligible and the Rules entry for 705430 narrows the event to Withering Touch's ranks.
DELETE FROM `spell_proc` WHERE `SpellId` = 705430;
INSERT INTO `spell_proc`
  (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`,
   `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`,
   `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`)
VALUES
  (705430, 0, 0, 0, 0, 0, 69652, 7, 2, 0, 0, 0, 0, 100, 0, 0);

DELETE FROM `spell_script_names` WHERE `spell_id` = 705430
  AND `ScriptName` = 'spell_ascension_reaper_talent_proc';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(705430, 'spell_ascension_reaper_talent_proc');
