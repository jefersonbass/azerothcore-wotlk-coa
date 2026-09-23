-- Plumes of War (705071), issue #2505: "Increases the critical strike chance of Falconstrike and
-- Emerald Arrow by $s2%. In addition, critical strikes with Falconstrike now increase the damage of
-- subsequent Falconstrikes by $705078s1%, stacking ..."
--
-- Measured before writing:
--   * The talent is half native and half dead. Its effects are auras 108 and 107 - the crit chance the
--     first sentence promises, which already works - plus a live SPELL_AURA_PROC_TRIGGER_SPELL (aura 42)
--     on 705078 that never fired: the record's Spell.dbc ProcFlags are 0 and the world DB holds no
--     `spell_proc` row, so SpellMgr::LoadSpellProcs skips it ("Skip if no proc flags in DB") and
--     Aura::GetProcEffectMask returns 0 before any effect is examined.
--   * ProcChance is 100, so the aura is a marker whose event is a Falconstrike critical strike, and
--     Chance stays 0 so the loader keeps using it (SpellMgr.cpp:2122).
--   * 705078 is native: a single aura 108 on the caster with base points 4, not in the EffectNULL set.
--   * The talent is PASSIVE, so its aura is applied at learn (Player.cpp:3387) - the gate exists.
--   * The proc sentence names Falconstrike only, so the row has to cover that ability's 14 spells. The
--     mask cannot: six of them (573060, 573248, 573338, 573339, 573340, 806465) carry
--     SpellFamilyFlags [0, 0, 0] and the other eight (806345, 806437-806443) carry [0, 4194304, 0] - two
--     groups, one of them with no bit at all. The row therefore drops the mask (family 0) and lets
--     spell_ascension_spell_list_talent_proc filter by the rule's spell list, which carries all 14 -
--     the convention recorded at rev_20260921_40_ranger_dead_procs.sql:1354.
--   * Emerald Arrow is named in the first sentence only, and that half is the native aura, so it is not
--     part of the list.
--   * ProcFlags 69904 = the four damage-by-ability classes with no auto-attack bits, SpellTypeMask 1
--     (DAMAGE) and SpellPhaseMask 2 (HIT).
--
-- The Rules entry is added in AscensionSpellListTalentProcs.h (18 -> 19).
DELETE FROM `spell_proc` WHERE `SpellId` = 705071;
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(705071, 0, 0, 0, 0, 0, 69904, 1, 2, 0, 0, 0, 0, 0, 0, 0);

DELETE FROM `spell_script_names` WHERE `spell_id` = 705071 AND `ScriptName` = 'spell_ascension_spell_list_talent_proc';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(705071, 'spell_ascension_spell_list_talent_proc');
