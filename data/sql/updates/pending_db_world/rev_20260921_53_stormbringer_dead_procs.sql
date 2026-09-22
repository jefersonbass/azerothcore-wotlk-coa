-- Storm Chaser (300612): "Direct critical strikes increase your haste" - the aura-42 passive ships ProcFlags 0
-- and no `spell_proc` row, so the clause never fired while the payload stays authored (the aura-42 handler
-- casts the record's own TriggerSpell). Chance 0 defers to the record's ProcChance (33%).
-- ProcFlags 69972 = melee and ranged auto attacks plus the four direct damage spell classes, periodic
-- excluded - the reviewed shape for a "direct critical strikes" clause (see Marked for Death and Freedom in
-- rev_20260921_40_ranger_dead_procs.sql) - with HitMask 2 = PROC_HIT_CRITICAL.
-- Storm Soul (300591), Electric Field (300599), Perpetual Shock (300625) and Opportune Flux (300598) share
-- this shape but are absent from CharacterAdvancement.dbc, so no character can acquire them; a row would be
-- dead code and is deliberately not added.
DELETE FROM `spell_proc` WHERE `SpellId` = 300612;
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(300612, 0, 0, 0, 0, 0, 69972, 1, 2, 2, 0, 0, 0, 0, 0, 0);
