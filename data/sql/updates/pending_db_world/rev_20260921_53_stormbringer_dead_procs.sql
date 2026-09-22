-- Stormbringer talents whose proc auras ship without proc flags (Spell.dbc ProcFlags 0 and no `spell_proc`
-- row), so the clauses below never fired. Every payload is already authored and native: the aura-42 handler
-- casts each record's TriggerSpell, so only the proc event is restored here.
-- Chance stays 0 everywhere: LoadSpellProcs falls back to each record's own ProcChance, which is the
-- tooltip's value (Electric Field 10%, Storm Chaser 33%, Perpetual Shock 35%, Storm Soul 100%).
-- ProcFlags 69972 = melee and ranged auto attacks plus the four direct damage spell classes, periodic
-- excluded - the reviewed shape for a "direct critical strikes" clause (see Marked for Death and Freedom in
-- rev_20260921_40_ranger_dead_procs.sql) - with HitMask 2 = PROC_HIT_CRITICAL.
-- ProcFlags 69904 = the four direct damage spell classes only ("dealing direct spell damage", no weapon
-- strikes). ProcFlags 262144 = PROC_FLAG_DONE_PERIODIC for Electric Field's "periodic damage dealt".
-- Perpetual Shock keys family 22 mask0 2048, the flag of Shock itself (804020), so only Shock raises the
-- second cast (570054, whose repeat behaviour the class script already implements).
-- Storm Soul (300591) and Perpetual Shock (300625) also lack SPELL_ATTR0_PASSIVE; the class contract
-- re-marks them so the talent apply passes install the aura the rows above need.
-- Opportune Flux (300598) is the same shape but ships in rev_20260922_01_stormbringer_opportune_flux_proc.sql.
DELETE FROM `spell_proc` WHERE `SpellId` IN (300591, 300599, 300612, 300625);
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(300591, 0, 0, 0, 0, 0, 69972, 1, 2, 2, 0, 0, 0, 0, 0, 0),
(300599, 0, 0, 0, 0, 0, 262144, 1, 2, 0, 0, 0, 0, 0, 0, 0),
(300612, 0, 0, 0, 0, 0, 69904, 1, 2, 0, 0, 0, 0, 0, 0, 0),
(300625, 0, 22, 2048, 0, 0, 69904, 1, 2, 0, 0, 0, 0, 0, 0, 0);
