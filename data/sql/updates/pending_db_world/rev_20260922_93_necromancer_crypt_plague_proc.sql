-- Crypt Plague (500117): "Your Lichfrost and Crypt Swarm now apply Crypt Plague."
--
-- SpellMgr::LoadSpellProcs skips a record that carries no DBC proc flags ("Skip if no proc flags in
-- DBC", src/server/game/Spells/SpellMgr.cpp), so no fallback entry is generated; Aura::GetProcEffectMask
-- then returns 0 for any aura with no proc entry ("only auras with spell proc entry can trigger proc",
-- src/server/game/Spells/Auras/SpellAuras.cpp). 500117 ships ProcFlags 0, so its aura 42 - on TriggerSpell
-- 570132, the "Add 1 Crypt Plague" applier that lands 570131 - has never fired. Same defect and same shape
-- as rev_20260922_70_necromancer_rime_necromancer_proc.sql.
--
-- Damage clause, so SpellPhaseMask 2 (HIT) with the spell-only ProcFlags 69904
-- (PROC_FLAG_DONE_SPELL_MELEE_DMG_CLASS 16 | DONE_SPELL_RANGED_DMG_CLASS 256 |
-- DONE_SPELL_NONE_DMG_CLASS_NEG 4096 | DONE_SPELL_MAGIC_DMG_CLASS_NEG 65536), the shape already used for
-- Befouling (704727) whose clause also names Crypt Swarm. SpellTypeMask 1 (PROC_SPELL_TYPE_DAMAGE).
--
-- The two abilities share no family bit - Crypt Swarm's ranks disagree among themselves (500965 carries
-- SpellFamilyFlags[0] bit 1 while 501055, 800343 and 803530 carry none) and Lichfrost is
-- SpellFamilyFlags[2] bit 27 - so the row leaves the masks open and the spell list lives in
-- AscensionNecromancerTalentProcs.h, the same route the other four talents take.
DELETE FROM `spell_proc` WHERE `SpellId` = 500117;
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(500117, 0, 0, 0, 0, 0, 69904, 1, 2, 0, 0, 0, 0, 0, 0, 0);
DELETE FROM `spell_script_names` WHERE `spell_id` = 500117 AND `ScriptName` = 'spell_ascension_necromancer_talent_proc';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(500117, 'spell_ascension_necromancer_talent_proc');
