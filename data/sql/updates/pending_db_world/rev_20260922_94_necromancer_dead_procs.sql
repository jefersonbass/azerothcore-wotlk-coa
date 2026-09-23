-- Three Necromancer records whose aura 42 gate is dead because the DBC ships ProcFlags 0.
--
-- SpellMgr::LoadSpellProcs skips a record that carries no DBC proc flags ("Skip if no proc flags in
-- DBC", src/server/game/Spells/SpellMgr.cpp), so no fallback entry is generated; Aura::GetProcEffectMask
-- then returns 0 for any aura with no proc entry ("only auras with spell proc entry can trigger proc",
-- src/server/game/Spells/Auras/SpellAuras.cpp). Same defect and same shape as
-- rev_20260922_70_necromancer_rime_necromancer_proc.sql.
--
-- #3957 Forbidden Technique (561215): "Casting Command spells now reduces the cooldown of your Animates
--   by $/1000;302910S2 sec." Cast clause, so SpellPhaseMask 1 (PROC_SPELL_PHASE_CAST) with ProcFlags
--   87312 (PROC_FLAG_DONE_SPELL_MELEE_DMG_CLASS 16 | DONE_SPELL_RANGED_DMG_CLASS 256 |
--   DONE_SPELL_NONE_DMG_CLASS_POS 1024 | DONE_SPELL_NONE_DMG_CLASS_NEG 4096 |
--   DONE_SPELL_MAGIC_DMG_CLASS_POS 16384 | DONE_SPELL_MAGIC_DMG_CLASS_NEG 65536) and SpellTypeMask 7,
--   the cast shape recorded for Rotten (804689). The clause names every Command spell, and
--   AscensionNecromancer.h Command() is Family(info, 2, 16777216), so SpellFamilyName 29 with
--   SpellFamilyMask2 16777216 matches exactly that set.
--
-- #2364 Pandemic (704702, 704703): "Gives your direct damage effects a 33% chance to spread any active
--   Necromancer diseases to an additional nearby enemy." Damage clause, so ProcFlags 69904 and
--   SpellPhaseMask 2 (HIT) with SpellTypeMask 1; the record's own ProcChance of 33 stays in charge, and
--   "direct damage effects" names no ability, so the masks stay open.
--
-- #2346 Frigid Ward (801735): "Damage taken applies Deathchill to attackers and reduces their movement
--   speed." "Damage taken" makes this a TAKEN event, so ProcFlags 680 (PROC_FLAG_TAKEN_MELEE_AUTO_ATTACK
--   8 | TAKEN_SPELL_MELEE_DMG_CLASS 32 | TAKEN_RANGED_AUTO_ATTACK 128 | TAKEN_SPELL_RANGED_DMG_CLASS 512)
--   with SpellPhaseMask 2 (HIT) and SpellTypeMask 1, the same shape recorded for Wretched Bile (704705).
DELETE FROM `spell_proc` WHERE `SpellId` IN (561215, 704702, 704703, 801735);
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(561215, 0, 29, 0, 0, 16777216, 87312, 7, 1, 0, 0, 0, 0, 0, 0, 0),
(704702, 0, 0, 0, 0, 0, 69904, 1, 2, 0, 0, 0, 0, 0, 0, 0),
(704703, 0, 0, 0, 0, 0, 69904, 1, 2, 0, 0, 0, 0, 0, 0, 0),
(801735, 0, 0, 0, 0, 0, 680, 1, 2, 0, 0, 0, 0, 0, 0, 0);
