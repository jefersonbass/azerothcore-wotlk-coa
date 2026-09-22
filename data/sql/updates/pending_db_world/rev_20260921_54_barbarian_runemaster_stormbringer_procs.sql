-- Dead proc gates, same shape as rev_20260921_53: each talent ships an aura-42 passive with Spell.dbc
-- ProcFlags 0 and no `spell_proc` row, so its clause never fired while the payload stays authored (the
-- aura-42 handler casts the record's own TriggerSpell).
-- Chance stays 0 everywhere, so LoadSpellProcs falls back to each record's own ProcChance (Leystone Springs
-- 15%, the others 100%).
-- Leystone Springs (300581, Runemaster): "melee auto attacks now have a 15% chance" - ProcFlags 4 is
-- PROC_FLAG_DONE_MELEE_AUTO_ATTACK, outside the spell/phase masks, so those stay 0.
-- Magic Etchings (300582, Runemaster): "Dealing damage with Primordial Blast" - family 38 masks 0x400000 /
-- 0x100000 / 0x40 are Primordial Blast's own flags (502823/502825).
-- Dark Skies (300593, Stormbringer): "direct damage spells fail to critically strike" is the non-critical
-- half of a direct damage spell hit - ProcFlags 69904 (the four direct damage spell classes) with HitMask 1
-- = PROC_HIT_NORMAL. Its second half, "critically striking removes this effect", rides the buff itself
-- (680855): a crit proc whose script removes the stack (aura_ascension_stormbringer_dark_skies).
-- Brutality Blade (300493, Barbarian) shares this shape but is absent from CharacterAdvancement.dbc, so no
-- character can acquire it; a row and a passive re-mark would be dead code and are deliberately not added.
DELETE FROM `spell_proc` WHERE `SpellId` IN (300581, 300582, 300593, 680855);
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(300581, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0),
(300582, 0, 38, 4194304, 1048576, 64, 69904, 1, 2, 0, 0, 0, 0, 0, 0, 0),
(300593, 0, 0, 0, 0, 0, 69904, 1, 2, 1, 0, 0, 0, 0, 0, 0),
(680855, 0, 0, 0, 0, 0, 69904, 1, 2, 2, 0, 0, 0, 0, 0, 0);

DELETE FROM `spell_script_names` WHERE `spell_id` = 680855 AND `ScriptName` = 'aura_ascension_stormbringer_dark_skies';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(680855, 'aura_ascension_stormbringer_dark_skies');
