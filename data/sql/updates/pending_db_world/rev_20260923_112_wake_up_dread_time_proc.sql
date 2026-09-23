-- Wake Up, It's Dread Time (705429) never fired.
--
-- The talent promises that a critical strike grants Dread Time, whose next melee hit generates 10
-- Runic Power. The chain is authored but neither gate was supplied: the talent's own aura 42 carries
-- ProcTypeMask 0 and no spell_proc row, and Dread Time (572180) is the same shape - its trigger
-- 355463 (SPELL_EFFECT_ENERGIZE on Runic Power at +10) is armed, but nothing in the Spell.dbc applies
-- Dread Time at all and its aura has no proc flags either.
--
-- Two rows, and the second is what makes the first one's payload reachable. The talent's HitMask 2 =
-- PROC_HIT_CRITICAL is the same value the Dark Passage row uses for "direct critical strikes", because
-- the tooltip says critical strikes. Dread Time's row keeps every hit result so the next melee swing
-- of any kind spends it.
DELETE FROM `spell_proc` WHERE `SpellId` IN (705429, 572180);
INSERT INTO `spell_proc`
  (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`,
   `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`,
   `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`)
VALUES
  (705429, 0, 0, 0, 0, 0, 69652, 7, 2, 2, 0, 0, 0, 100, 0, 0),
  (572180, 0, 0, 0, 0, 0, 69652, 7, 2, 0, 0, 0, 0, 100, 0, 0);

DELETE FROM `spell_script_names` WHERE `spell_id` = 705429
  AND `ScriptName` = 'aura_ascension_reaper_wake_up';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(705429, 'aura_ascension_reaper_wake_up');
