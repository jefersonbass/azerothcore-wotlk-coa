-- Chitinous Spikes proc (#2920): non-periodic critical strikes grant the spikes.
-- Talent 705987 (Proc Trigger Spell, "Causes non-periodic critical strikes to grant
-- Chitinous Spikes") ships without a spell_proc row, so the aura never fires and the
-- buff 705986 (AP of armor + damage shield, both native) is never applied. Row shape mirrors the sibling crit proc 705982 in rev_20260910_14 (all direct damage,
-- HitMask 2 = PROC_HIT_CRITICAL); the script narrows it to non-periodic crits.
START TRANSACTION;
DELETE FROM `spell_proc` WHERE `SpellId` = 705987;
INSERT INTO `spell_proc` (`SpellId`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `Chance`, `Charges`) VALUES
(705987, 1048575, 7, 2, 2, 0, 100, 0);
DELETE FROM `spell_script_names` WHERE `spell_id` = 705987 AND `ScriptName` = 'aura_ascension_venomancer_event';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (705987, 'aura_ascension_venomancer_event');
COMMIT;
