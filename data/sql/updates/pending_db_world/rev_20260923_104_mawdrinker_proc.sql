-- Mawdrinker (300561) never fired.
--
-- The talent promises that Requiem reduces damage taken by 5% for 8 seconds. Its record is a
-- SPELL_AURA_PROC_TRIGGER_SPELL whose trigger (300562) already applies the 5% reduction
-- (SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN at -6, DurationIndex 31 = 8000 ms) - the payload is authored,
-- but the record carries ProcTypeMask 0 and the talent had no spell_proc row, so the aura was built
-- with no proc flags and nothing could reach the trigger.
--
-- The row mirrors the shape its sibling Soulrot (805196) carries. SpellTypeMask 7 keeps every spell
-- type eligible and the Rules entry for 300561 narrows the event to Requiem's eight ranks.
DELETE FROM `spell_proc` WHERE `SpellId` = 300561;
INSERT INTO `spell_proc`
  (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`,
   `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`,
   `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`)
VALUES
  (300561, 0, 0, 0, 0, 0, 69652, 7, 2, 0, 0, 0, 0, 100, 0, 0);

DELETE FROM `spell_script_names` WHERE `spell_id` = 300561
  AND `ScriptName` = 'spell_ascension_reaper_talent_proc';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(300561, 'spell_ascension_reaper_talent_proc');
