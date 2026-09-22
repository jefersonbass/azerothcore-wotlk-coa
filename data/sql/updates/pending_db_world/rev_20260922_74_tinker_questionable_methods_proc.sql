-- Questionable Methods (706254): "Magic-Cleanser 4000X now also dispels 1 poison effect and heals an
-- ally for 3% of their maximum health."
--
-- SpellMgr::LoadSpellProcs skips a record that carries no DBC proc flags ("Skip if no proc flags in
-- DBC", src/server/game/Spells/SpellMgr.cpp), so no fallback entry is generated; Aura::GetProcEffectMask
-- then returns 0 for any aura with no proc entry ("only auras with spell proc entry can trigger proc",
-- src/server/game/Spells/Auras/SpellAuras.cpp). 706254 ships ProcFlags 0, so its effect 1 - aura 42
-- (SPELL_AURA_PROC_TRIGGER_SPELL), effect type 6, on TriggerSpell 520280 - has never fired.
-- The payload is authored: 520280 carries effect 136 with MiscValue 4 (DISPEL_POISON in the core's
-- DispelType: MAGIC 1, CURSE 2, DISEASE 3, POISON 4) and BasePoints 2, which is the tooltip's dispel
-- plus the 3% heal. Only the gate is missing. Same defect and same shape as
-- rev_20260920_48_bloodmage_ultra_instinct_proc.sql.
--
-- Cast shape, because the clause is "Magic-Cleanser 4000X now ALSO dispels" - it describes what happens
-- when that ability is used, not damage it deals: ProcFlags 87312 (PROC_FLAG_DONE_SPELL_MELEE_DMG_CLASS 16
--   | DONE_SPELL_RANGED_DMG_CLASS 256 | DONE_SPELL_NONE_DMG_CLASS_POS 1024 |
--   DONE_SPELL_NONE_DMG_CLASS_NEG 4096 | DONE_SPELL_MAGIC_DMG_CLASS_POS 16384 |
--   DONE_SPELL_MAGIC_DMG_CLASS_NEG 65536) with SpellTypeMask 7 and SpellPhaseMask 1
--   (PROC_SPELL_PHASE_CAST).
-- SpellTypeMask is never 0 while ProcFlags carries spell bits: the loader logs an error and the proc does
-- not fire.
--
-- The mask is zero and the family is zero on purpose. Magic-Cleanser 4000X (560472) carries family 34
-- word 0 bit 0, and that bit is not exclusive: four distinct names hold it (Aether Augmentation, Magic
-- Augmentation, Magic-Cleanser 4000X, Piercing Augmentation), so a mask would fire the proc on all four.
-- The narrowing lives in spell_ascension_spell_list_talent_proc instead, whose Rule for 706254 names
-- 560472 exactly.
-- Chance stays 0 so the record's own ProcChance (100) is used, per
-- rev_20260919_20_coa_proc_chance_parity.sql.
DELETE FROM `spell_proc` WHERE `SpellId` = 706254;
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(706254, 0, 0, 0, 0, 0, 87312, 7, 1, 0, 0, 0, 0, 0, 0, 0);

DELETE FROM `spell_script_names` WHERE `spell_id` = 706254 AND `ScriptName` = 'spell_ascension_spell_list_talent_proc';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(706254, 'spell_ascension_spell_list_talent_proc');
