-- Runemaster and Stormbringer talents whose proc auras ship without proc flags (Spell.dbc ProcFlags 0
-- and no `spell_proc` row), so the clauses below never fired. Payloads are already authored and native:
-- the aura-42 handler casts each record's TriggerSpell (Kinetic Energy 806410, Ancient Warrior 520757
-- cutting 2 sec from Fist of the Ancients 712326, Decoder 707152, Leyfrost 712464, Magebreaker 704430
-- applying its -15% magic-damage and dispel). Chance stays 0 everywhere, deferring to each record's own
-- ProcChance (Kinetic Energy 100, Ancient Warrior 100, Decoder 20, Leyfrost 100, Magebreaker 100).
-- SpellPhaseMask follows the tooltip verb: "Casting ..." / "whenever you use ..." fire on
-- PROC_SPELL_PHASE_CAST (1) so they raise once per cast, while "critical strikes with Hoarfrost" and
-- "dealing damage with Hurricane" fire on PROC_SPELL_PHASE_HIT (2), the former with HitMask 2
-- (PROC_HIT_CRITICAL). Each clause names the abilities that may raise it, so SpellFamilyMask keys their
-- own Spell.dbc family flags: Torrential Wrath (0/1/32) OR Deluge (0/32768/0); Runeblade (0/0/262144) OR
-- Primordial Blast (4194304/1048576/64); Elemental Burst (0/0/131072) OR Runeblade; Hoarfrost
-- (8388608/0/0); Hurricane (256/0/0) OR (0/0/260).
-- ProcFlags 69904 = the four direct damage spell classes only; 69972 adds melee and ranged auto attacks.
-- SpellTypeMask 1 = PROC_SPELL_TYPE_DAMAGE.
DELETE FROM `spell_proc` WHERE `SpellId` IN (806409, 520755, 706523, 712308, 804061);
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(806409, 0, 0, 0, 32769, 32, 69904, 1, 1, 0, 0, 0, 0, 0, 0, 0),
(520755, 0, 0, 4194304, 1048576, 262208, 69904, 1, 1, 0, 0, 0, 0, 0, 0, 0),
(706523, 0, 0, 0, 0, 393216, 69904, 1, 2, 0, 0, 0, 0, 0, 0, 0),
(712308, 0, 0, 8388608, 0, 0, 69972, 1, 2, 2, 0, 0, 0, 0, 0, 0),
(804061, 0, 0, 256, 0, 260, 69904, 1, 2, 0, 0, 0, 0, 0, 0, 0);
