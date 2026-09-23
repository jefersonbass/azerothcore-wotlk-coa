-- Hungering Crescent (704557) never fired.
--
-- The talent promises that Crow's Harvest increases the duration of Harvest Time by 1.5 seconds. Its
-- record is a SPELL_AURA_PROC_TRIGGER_SPELL whose trigger (560497) already carries an
-- ASCENSION_MODIFY_AURA_DURATION effect on Harvest Time (803995) at +1500 ms - exactly the 1.5
-- seconds the tooltip promises - but the record carries ProcTypeMask 0 and the talent had no
-- spell_proc row, so the aura was built with no proc flags and nothing could reach the trigger.
--
-- The row mirrors the shape its sibling Soulrot (805196) carries. SpellTypeMask 7 keeps every spell
-- type eligible and the Rules entry for 704557 narrows the event to Crow's Harvest's eight ranks.
DELETE FROM `spell_proc` WHERE `SpellId` = 704557;
INSERT INTO `spell_proc`
  (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`,
   `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`,
   `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`)
VALUES
  (704557, 0, 0, 0, 0, 0, 69652, 7, 2, 0, 0, 0, 0, 100, 0, 0);

DELETE FROM `spell_script_names` WHERE `spell_id` = 704557
  AND `ScriptName` = 'spell_ascension_reaper_talent_proc';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(704557, 'spell_ascension_reaper_talent_proc');
