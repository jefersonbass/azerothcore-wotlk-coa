-- Four Barbarian talents whose proc gate is dead, all narrowed by
-- spell_ascension_barbarian_talent_proc because the ability each tooltip names either carries no
-- family bit (Savage Strike, Unbridled Rage's appliers, Berserker Rush ranks) or shares one with an
-- ability the tooltip does not name.
--
-- SpellMgr::LoadSpellProcs skips a record that carries no DBC proc flags ("Skip if no proc flags in
-- DBC", src/server/game/Spells/SpellMgr.cpp), so no fallback entry is generated; Aura::GetProcEffectMask
-- then returns 0 for any aura with no proc entry ("only auras with spell proc entry can trigger proc",
-- src/server/game/Spells/Auras/SpellAuras.cpp). Every spell below ships ProcFlags 0, so its aura 42 has
-- never fired. Same defect and same shape as rev_20260920_48_bloodmage_ultra_instinct_proc.sql.
--
-- #2570 Savageness (705223): "Damage dealt by Savage Strike removes 2 stacks of Ripping Frenzy."
--   The clause is damage dealt, so SpellPhaseMask 2 (HIT) with ProcFlags 69904
--   (PROC_FLAG_DONE_SPELL_MELEE_DMG_CLASS 16 | DONE_SPELL_RANGED_DMG_CLASS 256 |
--   DONE_SPELL_NONE_DMG_CLASS_NEG 4096 | DONE_SPELL_MAGIC_DMG_CLASS_NEG 65536 - the spell-only set).
--   Savage Strike (514050-514055) carries no family flags at all, so no mask can select it.
--
-- #2545 Fueled by Rage (705159): "Whenever you gain Unbridled Rage, reduce the cooldown of your
--   Ancestral Roar by 2 sec." The gain arrives as the Unbridled Rage applier landing on the caster, so
--   the clause is a self-buff: ProcFlags 87312 with SpellTypeMask 7 and SpellPhaseMask 1 (CAST).
--
-- #2581 Savage Blood (705245): "Berserker Rush now grants Bloodfury." Casting an ability on yourself -
--   same self-buff shape (87312 / 7 / 1). Berserker Rush ranks 560518 and 561010-561013 carry family
--   word 1 bit 23, but 560519 and 560553 share the name with no flags, so the script holds the list.
--
-- #2314 Juggernaut (704591): "Whirling Advance now stuns affected enemies for 3.50 sec. Can only occur
--   once every 30 sec." Cast shape again (87312 / 7 / 1); Cooldown 30 carries the tooltip's own limit,
--   which the DBC does not encode.
--
-- SpellTypeMask is never 0 while ProcFlags carries spell bits: the loader logs an error and the proc does
-- not fire. Chance stays 0 so each record's own ProcChance is used, per
-- rev_20260919_20_coa_proc_chance_parity.sql.
DELETE FROM `spell_proc` WHERE `SpellId` IN (705223, 705159, 705245, 704591);
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(705223, 0, 0, 0, 0, 0, 69904, 1, 2, 0, 0, 0, 0, 0, 0, 0),
(705159, 0, 0, 0, 0, 0, 87312, 7, 1, 0, 0, 0, 0, 0, 0, 0),
(705245, 0, 0, 0, 0, 0, 87312, 7, 1, 0, 0, 0, 0, 0, 0, 0),
(704591, 0, 0, 0, 0, 0, 87312, 7, 1, 0, 0, 0, 0, 0, 30, 0);

DELETE FROM `spell_script_names` WHERE `ScriptName` = 'spell_ascension_barbarian_talent_proc';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(705223, 'spell_ascension_barbarian_talent_proc'),
(705159, 'spell_ascension_barbarian_talent_proc'),
(705245, 'spell_ascension_barbarian_talent_proc'),
(704591, 'spell_ascension_barbarian_talent_proc');
