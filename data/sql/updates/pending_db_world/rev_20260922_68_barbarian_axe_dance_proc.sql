-- Axe Dance (705224): "Ancestral Strike now costs 20 less Energy and knocks the target and you away
-- from each other, but now has a 20 sec cooldown."
--
-- SpellMgr::LoadSpellProcs skips a record that carries no DBC proc flags ("Skip if no proc flags in
-- DBC", src/server/game/Spells/SpellMgr.cpp), so no fallback entry is generated; Aura::GetProcEffectMask
-- then returns 0 for any aura with no proc entry ("only auras with spell proc entry can trigger proc",
-- src/server/game/Spells/Auras/SpellAuras.cpp). 705224 ships ProcFlags 0, so its aura 42 on TriggerSpell
-- 706500 has never fired.
-- The knockback payload is authored: 706500 carries effect 138 (MiscValue 125) and effect 98 (MiscValue
-- 75, MiscValueB 50). The other two halves of the tooltip are already live - effects 1 and 2 are
-- SPELL_AURA_ADD_PCT_MODIFIER carrying SPELLMOD_COST and SPELLMOD_COOLDOWN, both keyed to Ancestral
-- Strike's family bit (word 1, bit 20). Only the proc gate was missing.
--
-- Cast shape, because the clause describes what happens when Ancestral Strike is used, not damage it
-- deals: ProcFlags 87312 (PROC_FLAG_DONE_SPELL_MELEE_DMG_CLASS 16 | DONE_SPELL_RANGED_DMG_CLASS 256 |
--   DONE_SPELL_NONE_DMG_CLASS_POS 1024 | DONE_SPELL_NONE_DMG_CLASS_NEG 4096 |
--   DONE_SPELL_MAGIC_DMG_CLASS_POS 16384 | DONE_SPELL_MAGIC_DMG_CLASS_NEG 65536) with SpellTypeMask 7 and
--   SpellPhaseMask 1 (PROC_SPELL_PHASE_CAST).
-- SpellTypeMask is never 0 while ProcFlags carries spell bits: the loader logs an error and the proc does
-- not fire.
--
-- The narrowing lives in spell_ascension_barbarian_talent_proc rather than in a mask so the list can name
-- Ancestral Strike's nine records exactly (578267, 801576, 802444-802450). Its family bit is exclusive, but
-- a script entry keeps every Barbarian talent on one mechanism.
-- Chance stays 0 so the record's own ProcChance (100) is used, per
-- rev_20260919_20_coa_proc_chance_parity.sql.
DELETE FROM `spell_proc` WHERE `SpellId` = 705224;
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(705224, 0, 0, 0, 0, 0, 87312, 7, 1, 0, 0, 0, 0, 0, 0, 0);

DELETE FROM `spell_script_names` WHERE `spell_id` = 705224 AND `ScriptName` = 'spell_ascension_barbarian_talent_proc';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(705224, 'spell_ascension_barbarian_talent_proc');
