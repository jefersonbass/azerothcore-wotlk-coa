-- Witch Hunter talents whose proc auras ship without proc flags (Spell.dbc ProcFlags 0 and no `spell_proc`
-- row), so the clause never fired while the payload stays authored: the aura-42 handler casts each record's
-- TriggerSpell, and every payload here is family 21 like the talent (no cross-class trigger).
-- Chance stays 0 everywhere, so LoadSpellProcs falls back to each record's own ProcChance (Lord of Torture
-- 20%, the others 100%).
-- Lord of Torture (500099): "Periodic damage dealt now has a 20% chance" - the subject is damage DEALT, so
-- ProcFlags 262144 = PROC_FLAG_DONE_PERIODIC with SpellTypeMask 1 (damage) and SpellPhaseMask 2 (hit).
-- Create Distance (500901): "Casting Daring Escape" is a cast clause - SpellPhaseMask 1 = PROC_SPELL_PHASE_CAST
-- - keyed to Daring Escape (500086, family 21 flags[1] 0x80000000). SpellTypeMask 7 (all types) because an
-- escape cast is not a damaging event.
-- Bane of Witches (705451): "Quickdraw silences the target" - a hit clause (SpellPhaseMask 2) keyed to
-- Quickdraw (804193/806846, family 21 flags[0] 0x40 | flags[1] 0x20).
-- Dusk and Dawn (705483): "Damage dealt by Dawn Blade and Dusk Blade" - ProcFlags 69904 (the four direct
-- damage spell classes) with both blades in the mask (Dawn Blade 574342/802024 flags[0] 0x400, Dusk Blade
-- 802020 flags[0] 0x200).
DELETE FROM `spell_proc` WHERE `SpellId` IN (500099, 500901, 705451, 705483);
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(500099, 0, 0, 0, 0, 0, 262144, 1, 2, 0, 0, 0, 0, 0, 0, 0),
(500901, 0, 21, 0, 2147483648, 0, 69904, 7, 1, 0, 0, 0, 0, 0, 0, 0),
(705451, 0, 21, 64, 32, 0, 69904, 1, 2, 0, 0, 0, 0, 0, 0, 0),
(705483, 0, 21, 1536, 0, 0, 69904, 1, 2, 0, 0, 0, 0, 0, 0, 0);
