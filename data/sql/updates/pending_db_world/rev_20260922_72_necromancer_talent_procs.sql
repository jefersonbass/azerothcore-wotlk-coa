-- Four Necromancer talents whose proc gate is dead, narrowed by
-- spell_ascension_necromancer_talent_proc because three of them name an ability whose family bit is shared
-- or absent, and the fourth is a state-bound clause.
--
-- SpellMgr::LoadSpellProcs skips a record that carries no DBC proc flags ("Skip if no proc flags in
-- DBC", src/server/game/Spells/SpellMgr.cpp), so no fallback entry is generated; Aura::GetProcEffectMask
-- then returns 0 for any aura with no proc entry ("only auras with spell proc entry can trigger proc",
-- src/server/game/Spells/Auras/SpellAuras.cpp). Every spell below ships ProcFlags 0, so its aura 42 has
-- never fired. Same defect and same shape as rev_20260920_48_bloodmage_ultra_instinct_proc.sql.
--
-- #2350 Flesh To Worms RP GEN (704683): "Damage dealt with Flesh to Worms now generates 3 to 7 Runic
--   Power." Damage clause, so SpellPhaseMask 2 (HIT) with the spell-only ProcFlags 69904
--   (PROC_FLAG_DONE_SPELL_MELEE_DMG_CLASS 16 | DONE_SPELL_RANGED_DMG_CLASS 256 |
--   DONE_SPELL_NONE_DMG_CLASS_NEG 4096 | DONE_SPELL_MAGIC_DMG_CLASS_NEG 65536).
--
-- #2375 Befouling (704727): "Dealing damage with Crypt Swarm now has a 10% chance to reduce the cost of
--   Command: Gargoyle by 50% for 5 sec." Same damage clause and ProcFlags; the record's own ProcChance
--   of 10 stays in charge.
--
-- #3428 Rotten (804689): "Causes Putrefy to apply Putrid to your target." The clause describes what
--   happens when Putrefy is used, so it takes the cast shape: ProcFlags 87312
--   (PROC_FLAG_DONE_SPELL_MELEE_DMG_CLASS 16 | DONE_SPELL_RANGED_DMG_CLASS 256 |
--   DONE_SPELL_NONE_DMG_CLASS_POS 1024 | DONE_SPELL_NONE_DMG_CLASS_NEG 4096 |
--   DONE_SPELL_MAGIC_DMG_CLASS_POS 16384 | DONE_SPELL_MAGIC_DMG_CLASS_NEG 65536) with SpellTypeMask 7 and
--   SpellPhaseMask 1 (PROC_SPELL_PHASE_CAST).
--
-- #2366 Wretched Bile (704705): "While your Fetid Shield is active, melee or ranged damage now has a 40%
--   chance to afflict the attacker with Wretched Bile." "Afflict the attacker" makes this a TAKEN event on
--   the victim, not a DONE one, so ProcFlags 680 = PROC_FLAG_TAKEN_MELEE_AUTO_ATTACK (8) |
--   PROC_FLAG_TAKEN_SPELL_MELEE_DMG_CLASS (32) | PROC_FLAG_TAKEN_RANGED_AUTO_ATTACK (128) |
--   PROC_FLAG_TAKEN_SPELL_RANGED_DMG_CLASS (512). The "while Fetid Shield is active" half has no ProcFlag
--   equivalent, so the script holds RequiredAura 680986 and the rule's spell list stays open.
--
-- SpellTypeMask is never 0 while ProcFlags carries spell bits: the loader logs an error and the proc does
-- not fire. Chance stays 0 so each record's own ProcChance is used, per
-- rev_20260919_20_coa_proc_chance_parity.sql.
DELETE FROM `spell_proc` WHERE `SpellId` IN (704683, 704727, 804689, 704705);
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(704683, 0, 0, 0, 0, 0, 69904, 1, 2, 0, 0, 0, 0, 0, 0, 0),
(704727, 0, 0, 0, 0, 0, 69904, 1, 2, 0, 0, 0, 0, 0, 0, 0),
(804689, 0, 0, 0, 0, 0, 87312, 7, 1, 0, 0, 0, 0, 0, 0, 0),
(704705, 0, 0, 0, 0, 0, 680, 1, 2, 0, 0, 0, 0, 0, 0, 0);

DELETE FROM `spell_script_names` WHERE `ScriptName` = 'spell_ascension_necromancer_talent_proc';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(704683, 'spell_ascension_necromancer_talent_proc'),
(704727, 'spell_ascension_necromancer_talent_proc'),
(804689, 'spell_ascension_necromancer_talent_proc'),
(704705, 'spell_ascension_necromancer_talent_proc');
