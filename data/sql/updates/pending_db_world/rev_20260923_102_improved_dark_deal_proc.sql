-- Improved Dark Deal (300555) never fired.
--
-- The talent promises that parrying or dodging an attack reduces Dark Deal's cooldown by 1 second,
-- at most once every second. Its record is a SPELL_AURA_PROC_TRIGGER_SPELL whose trigger (300556) is
-- an ASCENSION_MODIFY_COOLDOWN effect on Dark Deal (803996) with -1000 ms - exactly the second the
-- tooltip promises - but the record carries ProcTypeMask 0 and the talent had no spell_proc row, so
-- the aura was built with no proc flags and nothing could reach the trigger.
--
-- ProcFlags 680 = TAKEN_MELEE_AUTO_ATTACK | TAKEN_SPELL_MELEE_DMG_CLASS | TAKEN_RANGED_AUTO_ATTACK |
-- TAKEN_SPELL_RANGED_DMG_CLASS: only physical attacks can be parried or dodged. HitMask 48 =
-- PROC_HIT_DODGE | PROC_HIT_PARRY (Unit.cpp sets those bits from the melee outcome), and the row's
-- Cooldown of 1000 ms is the "only once every 1 second" clause.
DELETE FROM `spell_proc` WHERE `SpellId` = 300555;
INSERT INTO `spell_proc`
  (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`,
   `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`,
   `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`)
VALUES
  (300555, 0, 0, 0, 0, 0, 680, 1, 2, 48, 0, 0, 0, 100, 1000, 0);

DELETE FROM `spell_script_names` WHERE `spell_id` = 300555
  AND `ScriptName` = 'spell_ascension_reaper_talent_proc';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(300555, 'spell_ascension_reaper_talent_proc');
