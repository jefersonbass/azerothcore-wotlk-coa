-- Hard Bargain (300569) never fired.
--
-- The talent promises that while Tormented Souls is active on the caster, melee attack speed rises by
-- 30% and Dreadwake costs less Runic Power. Its record is a SPELL_AURA_PROC_TRIGGER_SPELL whose
-- trigger 572300 already carries both halves - an aura at +29 base points for the attack speed and an
-- ADD_FLAT_MODIFIER for the cost - but the record carries ProcTypeMask 0 and the talent had no
-- spell_proc row, so the aura was built with no proc flags and nothing could reach the trigger.
--
-- The condition is a state, not a spell event, so the row stays broad and the script checks
-- Tormented Souls (500481) before casting the payload - the same shape the Ruin and Redshade rows use.
DELETE FROM `spell_proc` WHERE `SpellId` = 300569;
INSERT INTO `spell_proc`
  (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`,
   `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`,
   `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`)
VALUES
  (300569, 0, 0, 0, 0, 0, 69652, 7, 2, 0, 0, 0, 0, 100, 0, 0);

DELETE FROM `spell_script_names` WHERE `spell_id` = 300569
  AND `ScriptName` = 'aura_ascension_reaper_hard_bargain';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(300569, 'aura_ascension_reaper_hard_bargain');
