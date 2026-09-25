-- Restores the Terrasmash (706220) proc row to the shape rev_1789904506505340400 wrote.
--
-- That file carries the newer commit, but the updater sorts the pending files lexically (DBUpdater.cpp,
-- std::sort over the file names), so rev_1789... ran first and rev_20260918_30_primalist_mountain_procs
-- ran after it. The older file deleted every 706220 row and reinserted one with the mask columns zeroed,
-- which dropped the SpellTypeMask, SpellPhaseMask, HitMask, AttributesMask and DisableEffectsMask the
-- newer commit had set. The zeroed row is the one that survived.
--
-- The older file no longer writes 706220, so a fresh database gets the newer row directly; this file
-- repairs a database that already ran the pair in the older order.
DELETE FROM `spell_proc` WHERE `SpellId` = 706220;
INSERT INTO `spell_proc`
(`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`,
 `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`,
 `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(706220, 0, 0, 0, 0, 0, 8388608, 1, 2, 3, 2, 4, 0, 30, 0, 0);
