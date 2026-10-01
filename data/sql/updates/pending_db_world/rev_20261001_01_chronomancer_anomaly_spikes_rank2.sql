-- Anomaly Spikes rank 2 (504886): same missing proc gate as rank 1 (503825,
-- rev_20260919_86_chronomancer_anomaly_spikes_proc.sql). Spell.dbc gives 504886
-- ProcFlags 0 and ProcChance 15 with trigger 503826, so without a row the spike
-- is never launched for rank 2 holders. Masks mirror rank 1: the tooltip names
-- no ability ("periodic damage dealt"), the tick carries damage (type 1, phase
-- hit, DONE_PERIODIC 262144), and the payload is direct damage (no re-entry).
DELETE FROM `spell_proc` WHERE `SpellId` = 504886;
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(504886, 0, 0, 0, 0, 0, 262144, 1, 2, 0, 0, 0, 0, 15, 0, 0);
