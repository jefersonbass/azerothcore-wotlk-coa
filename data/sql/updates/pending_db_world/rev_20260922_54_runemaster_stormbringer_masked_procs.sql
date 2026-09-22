-- Runemaster and Stormbringer talents whose proc auras ship without proc flags (Spell.dbc ProcFlags 0
-- and no `spell_proc` row), so the clauses below never fired. Payloads are already authored and native:
-- the aura-42 handler casts each record's TriggerSpell. Chance stays 0 everywhere, deferring to each
-- record's own ProcChance (Conjuration Mastery 100, Never Strikes Twice? 25, Cold Elements 20,
-- Primordial Echoes 40).
-- Each clause names the abilities that may raise it, so SpellFamilyMask keys exactly those abilities'
-- Spell.dbc family flags: Conjure Storm word2 48; Electrocute word0 33554432 / word1 2097152 / word2 32;
-- Elemental Burst word2 131072; Primordial Blast word0 4194304 / word1 1048576 / word2 64 (the two are
-- OR-ed for the clause naming both).
-- ProcFlags 69904 = the four direct damage spell classes only; 69972 adds melee and ranged auto attacks
-- and is the reviewed shape for a critical-strike clause. HitMask 2 = PROC_HIT_CRITICAL for Primordial
-- Echoes' "critical strikes dealt with Primordial Blast", HitMask 0 = any hit elsewhere.
-- SpellTypeMask 1 = PROC_SPELL_TYPE_DAMAGE, SpellPhaseMask 2 = PROC_SPELL_PHASE_HIT.
-- Frost Glyph (500309) belongs in this file after all: it is granted through SkillLineAbility
-- (SkillLine 116, "Glyphic"), so it is reachable.
-- SpellFamilyName is set on every row that carries a mask: SpellInfo::IsAffected returns true as soon
-- as the family name is 0, so a mask without its family is never consulted. 22 is Stormbringer,
-- 38 is Runemaster.
DELETE FROM `spell_proc` WHERE `SpellId` IN (300595, 804828, 520138, 705572, 500309);
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(300595, 0, 22, 0, 0, 48, 69904, 1, 2, 0, 0, 0, 0, 0, 0, 0),
(804828, 0, 22, 33554432, 2097152, 32, 69904, 1, 2, 0, 0, 0, 0, 0, 0, 0),
(520138, 0, 38, 4194304, 1048576, 131136, 69904, 1, 2, 0, 0, 0, 0, 0, 0, 0),
(705572, 0, 38, 4194304, 1048576, 64, 69972, 1, 2, 2, 0, 0, 0, 0, 0, 0),
(500309, 0, 38, 4194304, 1048576, 131136, 69904, 1, 2, 0, 0, 0, 0, 0, 0, 0);
