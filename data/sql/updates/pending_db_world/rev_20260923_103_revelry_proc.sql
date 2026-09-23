-- Revelry (704550) never fired.
--
-- The talent promises that taking damage reduces Ghastly Screech's remaining cooldown by 3 seconds,
-- at most once every second. Its record is a SPELL_AURA_PROC_TRIGGER_SPELL whose trigger (704551) is
-- an ASCENSION_MODIFY_COOLDOWN effect on Ghastly Screech (806146) with -3000 ms - exactly the three
-- seconds the tooltip promises - but the record carries ProcTypeMask 0 and the talent had no
-- spell_proc row, so the aura was built with no proc flags and nothing could reach the trigger.
--
-- ProcFlags 8872 = the four TAKEN physical flags (680) plus TAKEN_SPELL_NONE_DMG_CLASS_NEG (0x2000)
-- and TAKEN_SPELL_MAGIC_DMG_CLASS_NEG (0x20000): "taking damage" is not restricted to physical hits.
-- Any hit result qualifies, so HitMask stays 0, and the row's Cooldown of 1000 ms is the "can only
-- occur once every sec" clause.
DELETE FROM `spell_proc` WHERE `SpellId` = 704550;
INSERT INTO `spell_proc`
  (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`,
   `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`,
   `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`)
VALUES
  (704550, 0, 0, 0, 0, 0, 8872, 1, 2, 0, 0, 0, 0, 100, 1000, 0);

DELETE FROM `spell_script_names` WHERE `spell_id` = 704550
  AND `ScriptName` = 'spell_ascension_reaper_talent_proc';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(704550, 'spell_ascension_reaper_talent_proc');
