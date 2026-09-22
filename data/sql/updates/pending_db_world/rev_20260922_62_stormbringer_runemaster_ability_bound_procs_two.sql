-- Stormbringer and Runemaster talents whose proc auras ship without proc flags (Spell.dbc ProcFlags 0
-- and no `spell_proc` row), so the clauses below never fired. Payloads are already authored and native:
-- the aura-42 handler casts each record's TriggerSpell (Conductive 567559, Gathered Energy 807130,
-- Fluxocute 707039, Riftblade 807003, Cosmic Impact 800738, Tablet of Sorcery 705604). Chance stays 0
-- everywhere, deferring to each record's own ProcChance (Stormsurge 15, the rest 100).
-- Every clause is "damage dealt by <ability>" / "causes <ability> to ...", so all six rows use the hit
-- phase with ProcFlags 69904 (the four direct damage spell classes) and SpellTypeMask 1.
-- SpellFamilyName is set on every row: SpellInfo::IsAffected returns true as soon as the family name is
-- 0, so a mask without its family is never consulted. 22 is Stormbringer, 38 is Runemaster.
-- SpellFamilyMask keys the ability the clause names, OR-ing every record of an ability that has more
-- than one: Stormflow 32/65536/0; Gale 16384/537001984/136; Electrocute 33554432/2097152/32;
-- Primordial Blast 4194304/1048576/64 OR Smolder 512/0/0 -> 4194816/1048576/64; Glyphic Ruin
-- 4096/0/0 (the same word the record's own crit-damage spellmod already keys); Fist of the Ancients
-- 0/0/32768 OR 0/0/33792 -> 0/0/33792.
-- Unmaker's first clause ("15% increased critical strike damage") is a native effect-1 spellmod that
-- already carries the Glyphic Ruin mask; only its second clause ("grant Cosmic Impact") rides this row.
DELETE FROM `spell_proc` WHERE `SpellId` IN (705673, 705679, 804597, 500314, 705631, 705603);
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(705673, 0, 22, 32, 65536, 0, 69904, 1, 2, 0, 0, 0, 0, 0, 0, 0),
(705679, 0, 22, 16384, 537001984, 136, 69904, 1, 2, 0, 0, 0, 0, 0, 0, 0),
(804597, 0, 22, 33554432, 2097152, 32, 69904, 1, 2, 0, 0, 0, 0, 0, 0, 0),
(500314, 0, 38, 4194816, 1048576, 64, 69904, 1, 2, 0, 0, 0, 0, 0, 0, 0),
(705631, 0, 38, 4096, 0, 0, 69904, 1, 2, 0, 0, 0, 0, 0, 0, 0),
(705603, 0, 38, 0, 0, 33792, 69904, 1, 2, 0, 0, 0, 0, 0, 0, 0);
