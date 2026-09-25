-- Stormbringer and Runemaster talents whose proc auras ship without proc flags (Spell.dbc ProcFlags 0
-- and no `spell_proc` row), so the clauses below never fired. Payloads are already authored and native:
-- the aura-42 handler casts each record's TriggerSpell (Kirin Tor Agent 706422, Invigoration 681273,
-- Altered Course 707219, Gift of Air 804033). Chance stays 0 everywhere, deferring to each record's own
-- ProcChance (all 100).
-- Every clause here reads "Casting <ability> now ..." / "Your <ability> now grants ...", so the rows use
-- PROC_SPELL_PHASE_CAST (1) and raise once per cast rather than once per target hit, following
-- rev_20260919_64. SpellFamilyMask keys the named ability's own Spell.dbc family flags: Eye of the
-- Beholder 0/4/0, Aeroblast 8388608/16/32, Discharge 0/4096/0, Kiss of the Clouds 0/0/1048576.
-- Ability-bound damage clauses use ProcFlags 69904 with SpellTypeMask 1; the utility casts (Eye of the
-- Beholder, Kiss of the Clouds) use 87312 with SpellTypeMask 7, the shape already used for cast-phase
-- rows such as rev_1789944986203842691.
-- Gift of Air's tooltip has a second clause - "your critical strikes now extend the duration of your
-- Tailwind by 1.5 sec" - which is not driven by this aura and is NOT restored here; only the
-- "Casting Kiss of the Clouds empowers your Air Elemental" clause rides this row.
-- MASK CORRECTION (2026-09-22) - THIS SUPERSEDES THE MASK VALUES NAMED ABOVE: SpellFamilyMask
-- matches by ANY shared bit (flag96 operator& plus
-- operator bool in Util.h:513/557, consumed at SpellInfo.cpp:1439), so a mask set to an ability full
-- flags also matched every ability sharing any of those bits - word2 bit 32 alone is shared by about
-- thirteen Stormbringer abilities. The rows here now carry only the bits that are EXCLUSIVE to the
-- ability the clause names, so the mask selects it alone. Where an ability has no exclusive bit at
-- all (its whole flag set is shared), the mask cannot isolate it and the row must be filtered by the
-- spell list in AscensionStormbringerRunemasterTalentProcs instead.
-- ROW REMOVAL (2026-09-24) - THIS SUPERSEDES THE 705700 AND 705715 ROWS THIS FILE USED TO WRITE:
-- both were restored to the shape their own gameplay scenarios assert, by rev_20260924_03 (which
-- sorts after this file). Invigoration 705700: stormbringer-fix-invigoration-stack-family.json names
-- the Aeroblast HIT, so the row needs SpellPhaseMask 2 and this file's phase 1 contradicted it.
-- Gift of Air 705715: stormbringer-fix-air-elemental-family-gift-of-air.json asserts the critical
-- strike extends Tailwind, which needs the HIT phase, so the row needs SpellPhaseMask 3 with HitMask
-- 3; the "not driven by this aura" note above is therefore superseded by that scenario.
-- The basis is the scenario assertion, not an independent measurement.
DELETE FROM `spell_proc` WHERE `SpellId` IN (705634, 707053);
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(705634, 0, 38, 0, 0, 0, 87312, 7, 1, 0, 0, 0, 0, 0, 0, 0),
(707053, 0, 22, 0, 0, 0, 69904, 1, 1, 0, 0, 0, 0, 0, 0, 0);

DELETE FROM `spell_script_names` WHERE `spell_id` IN (705634, 707053)
  AND `ScriptName` = 'spell_ascension_stormbringer_runemaster_talent_proc';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(705634, 'spell_ascension_stormbringer_runemaster_talent_proc'),
(707053, 'spell_ascension_stormbringer_runemaster_talent_proc');
