-- Counterscythe (805714) never fired its parry damage.
--
-- The buff already carries both halves of its parry rating natively - aura 189 SPELL_AURA_MOD_RATING
-- on combat rating 8 (parry) at +50, and aura 220 SPELL_AURA_MOD_RATING_FROM_STAT on the same rating
-- for the 25% of Stamina - and its aura 42 SPELL_AURA_PROC_TRIGGER_SPELL already names the damage
-- trigger 805715 (NORMALIZED_WEAPON_DMG plus WEAPON_PERCENT_DAMAGE at 100%). The record carries
-- ProcTypeMask 0 and the spell had no spell_proc row, so the aura was built with no proc flags and
-- nothing could reach the trigger.
--
-- ProcFlags 680 = the four TAKEN physical flags, the same set the Improved Dark Deal row uses.
-- HitMask 32 = PROC_HIT_PARRY only (SpellMgr.h:262): the tooltip says parries, not dodges, which is
-- why this row is 32 where Improved Dark Deal's "parrying or dodging" needed 48. SpellTypeMask 7
-- keeps every spell type eligible.
--
-- The 10 parry limit is the script below; the damage itself stays native, so the script only counts.
DELETE FROM `spell_proc` WHERE `SpellId` = 805714;
INSERT INTO `spell_proc`
  (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`,
   `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`,
   `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`)
VALUES
  (805714, 0, 0, 0, 0, 0, 680, 7, 2, 32, 0, 0, 0, 100, 0, 0);

DELETE FROM `spell_script_names` WHERE `spell_id` = 805714
  AND `ScriptName` = 'aura_ascension_counterscythe';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(805714, 'aura_ascension_counterscythe');
