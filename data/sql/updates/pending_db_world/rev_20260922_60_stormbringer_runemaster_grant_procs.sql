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
DELETE FROM `spell_proc` WHERE `SpellId` IN (705634, 705700, 707053, 705715);
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(705634, 0, 0, 0, 4, 0, 87312, 7, 1, 0, 0, 0, 0, 0, 0, 0),
(705700, 0, 0, 8388608, 16, 32, 69904, 1, 1, 0, 0, 0, 0, 0, 0, 0),
(707053, 0, 0, 0, 4096, 0, 69904, 1, 1, 0, 0, 0, 0, 0, 0, 0),
(705715, 0, 0, 0, 0, 1048576, 87312, 7, 1, 0, 0, 0, 0, 0, 0, 0);
