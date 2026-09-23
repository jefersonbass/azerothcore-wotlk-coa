-- Final Destination (705444) never fired.
--
-- The talent is a SPELL_AURA_PROC_TRIGGER_SPELL whose DBC record carries ProcTypeMask 0, so SpellMgr
-- builds the aura with no proc flags and nothing can reach its trigger (504167, the Wraithblade
-- cooldown reset). The payload is authored; only the gate is missing.
--
-- The row mirrors the one its sibling Soulrot (805196) already carries - ProcFlags 69652,
-- SpellTypeMask 1, SpellPhaseMask 2, Chance 100 - and the shared script decides which spell
-- qualifies: the Rules entry for 705444 lists Soulrot (804660), so only a Soulrot application passes.
DELETE FROM `spell_proc` WHERE `SpellId` = 705444;
INSERT INTO `spell_proc`
  (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`,
   `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`,
   `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`)
VALUES
  (705444, 0, 0, 0, 0, 0, 69652, 1, 2, 0, 0, 0, 0, 100, 0, 0);

DELETE FROM `spell_script_names` WHERE `spell_id` = 705444
  AND `ScriptName` = 'spell_ascension_reaper_talent_proc';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(705444, 'spell_ascension_reaper_talent_proc');
