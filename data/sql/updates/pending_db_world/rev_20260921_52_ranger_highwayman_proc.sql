-- Highwayman (707744): "Direct critical strikes while behind your target increase your haste by 15% for
-- 5 sec, or for your next 2 attacks."
-- Spell.dbc authors the talent as a passive (Attributes 0x101c0) whose single effect is aura 42 (proc trigger
-- spell) with TriggerSpell 705063 and ProcFlags 0, so the aura never procced. The payload chain is native:
-- 705063 triggers 704545 (aura 192 = SPELL_AURA_MOD_MELEE_RANGED_HASTE, bp 14 -> +15%, 5 sec) on its caster.
-- The row enables every direct damage critical strike -- melee and ranged auto attacks plus direct damage
-- spells, periodic excluded, matching the tooltip's "direct" -- with HitMask 2 = PROC_HIT_CRITICAL. ProcFlags
-- 69972 is the reviewed Ranger convention for a "direct critical strikes" clause (see Marked for Death 806973
-- and Freedom 520628 in rev_20260921_40_ranger_dead_procs.sql). AttributesMask 0: the proc source is the
-- player's own strike, never a triggered cast.
-- The "behind your target" gate has no proc flag, so aura_ascension_ranger_highwayman checks it and blocks
-- the proc otherwise.
-- The "next 2 attacks" clause is the companion row on 704545 (Charges 2) in rev_20260921_40_ranger_dead_procs.sql;
-- that row is inert until this one lets 707744 grant the buff at all.
DELETE FROM `spell_proc` WHERE `SpellId` = 707744;
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(707744, 0, 0, 0, 0, 0, 69972, 1, 2, 2, 0, 0, 0, 100, 0, 0);

DELETE FROM `spell_script_names` WHERE `spell_id` = 707744 AND `ScriptName` = 'aura_ascension_ranger_highwayman';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(707744, 'aura_ascension_ranger_highwayman');
