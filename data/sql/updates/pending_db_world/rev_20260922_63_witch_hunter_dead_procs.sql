-- Witch Hunter talents whose proc auras ship without proc flags (Spell.dbc ProcFlags 0 and no `spell_proc`
-- row), so the clause never fired while the payload stays authored: the aura-42 handler casts each record's
-- TriggerSpell, and every payload here is family 21 like the talent (no cross-class trigger).
-- Chance stays 0 everywhere, so LoadSpellProcs falls back to each record's own ProcChance (Lord of Torture
-- 20%, Create Distance 100%).
-- Mask rule: IsAffected (SpellInfo.cpp:1439) matches on ANY shared bit, so a row may only use bits that are
-- EXCLUSIVE to the abilities the tooltip names - otherwise it fires on every family spell sharing a bit.
-- Both masks below were checked with mask_scan.py: Create Distance's Daring Escape (500086, flags[1]
-- 0x80000000) is its only holder, and Lord of Torture is deliberately family 0 with no mask because the
-- clause is generic "periodic damage dealt".
-- Lord of Torture (500099): "Periodic damage dealt now has a 20% chance" - the subject is damage DEALT, so
-- ProcFlags 262144 = PROC_FLAG_DONE_PERIODIC with SpellTypeMask 1 (damage) and SpellPhaseMask 2 (hit).
-- Create Distance (500901): "Casting Daring Escape" is a cast clause - SpellPhaseMask 1 = PROC_SPELL_PHASE_CAST
-- with SpellTypeMask 7 (all types) because an escape cast is not a damaging event. ProcFlags 72464 is 69904
-- (the four direct damage classes) plus 2560 (the beneficial pair) - an escape is a positive spell, so its
-- cast event carries the positive flags rather than the negative ones.
-- NOT added: Bane of Witches (705451, Quickdraw) and Dusk and Dawn (705483, Dawn Blade + Dusk Blade). Quickdraw
-- (flags[0] 0x40 | flags[1] 0x20) shares both bits - the mask also selects Grasp of the Undying 680483 and
-- Sixfold Shot 807527. Dawn Blade (flags[0] 0x400) is shared with Surging Blade 681788, Witchblood Fever
-- 684330, Dark Peril 685020 and 686020, so the tooltip's pair cannot be isolated by a mask; both need the
-- mask-0 + spell-list script route.
DELETE FROM `spell_proc` WHERE `SpellId` IN (500099, 500901);
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(500099, 0, 0, 0, 0, 0, 262144, 1, 2, 0, 0, 0, 0, 0, 0, 0),
(500901, 0, 21, 0, 2147483648, 0, 72464, 7, 1, 0, 0, 0, 0, 0, 0, 0);
