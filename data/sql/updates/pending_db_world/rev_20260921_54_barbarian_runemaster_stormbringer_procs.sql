-- Dead proc gates, same shape as rev_20260921_53: each talent ships an aura-42 passive with Spell.dbc
-- ProcFlags 0 and no `spell_proc` row, so its clause never fired while the payload stays authored (the
-- aura-42 handler casts the record's own TriggerSpell).
-- Chance stays 0 everywhere, so LoadSpellProcs falls back to each record's own ProcChance (Leystone Springs
-- 15%, the others 100%).
-- All four ids are acquirable: Leystone Springs, Magic Etchings and Dark Skies sit in CharacterAdvancement,
-- Brutality Blade is taught through its class skill line (SkillLineAbility spell column: 300493 skill 41).
-- Brutality Blade (300493, Barbarian): "Dealing damage with Ancestral Strike or Brutal Swing" - family 18
-- mask1 1048578 keys both (Ancestral Strike 801576/802444.. flags[1] 0x100000, Brutal Swing 500913/500996..
-- flags[1] 0x2). The talent also lacks SPELL_ATTR0_PASSIVE; the Barbarian contract re-marks it so the apply
-- passes install the aura.
-- Leystone Springs (300581, Runemaster): "melee auto attacks now have a 15% chance" - ProcFlags 4 is
-- PROC_FLAG_DONE_MELEE_AUTO_ATTACK, outside the spell/phase masks, so those stay 0.
-- NOT added: Magic Etchings (300582, Runemaster). Its mask (Primordial Blast's own flags 0x400000 / 0x100000 /
-- 0x40) also selects Hydros 713002, Plasma Ball 802966 and Runic Obliteration 807014/807025, because
-- IsAffected (SpellInfo.cpp:1439) matches on ANY shared bit - the armour debuff would land from those
-- abilities too. Primordial Blast has no exclusive bit, so it needs the mask-0 + spell-list script route.
-- Dark Skies (300593, Stormbringer): "direct damage spells fail to critically strike" is the non-critical
-- half of a direct damage spell hit - ProcFlags 69904 (the four direct damage spell classes) with HitMask 1
-- = PROC_HIT_NORMAL. Its second half, "critically striking removes this effect", rides the buff itself
-- (680855): a crit proc whose script removes the stack (aura_ascension_stormbringer_dark_skies).
DELETE FROM `spell_proc` WHERE `SpellId` IN (300493, 300581, 300593, 680855);
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(300493, 0, 18, 0, 1048578, 0, 69904, 1, 2, 0, 0, 0, 0, 0, 0, 0),
(300581, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0),
(300593, 0, 0, 0, 0, 0, 69904, 1, 2, 1, 0, 0, 0, 0, 0, 0),
(680855, 0, 0, 0, 0, 0, 69904, 1, 2, 2, 0, 0, 0, 0, 0, 0);

DELETE FROM `spell_script_names` WHERE `spell_id` = 680855 AND `ScriptName` = 'aura_ascension_stormbringer_dark_skies';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(680855, 'aura_ascension_stormbringer_dark_skies');
