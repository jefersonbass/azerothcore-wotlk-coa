-- Stormbringer and Runemaster talents whose proc auras ship without proc flags (Spell.dbc ProcFlags 0
-- and no `spell_proc` row), so the clauses below never fired. Payloads are already authored and native:
-- the aura-42 handler casts each record's TriggerSpell (Runic Power 561056, Cloudy 520843, Warpdagger's
-- dispel 503734, Cyclone's Recharge 300828, Lord of Lightning 801841). Chance stays 0 everywhere,
-- deferring to each record's own ProcChance (all 100).
-- Runic Power's +3% raid damage is a separate native effect-0 aura (79 with misc 127); only its
-- "Primordial Blast now increases the chance for the target to be crippled" clause rides this proc.
-- Clauses that fire on casting (Ley Walker, Cyclone's Recharge, Storm Clouds, Lord of Lightning) use
-- SpellPhaseMask 1 (PROC_SPELL_PHASE_CAST) so they raise once per cast rather than once per target hit;
-- the Primordial Blast damage clause uses the hit phase. Self-buff and utility casts use ProcFlags 87312
-- (any spell carrying a damage class, positive or negative) with SpellTypeMask 7, the shape already used
-- for cast-phase rows such as rev_1789944986203842691; damage clauses use 69904 with SpellTypeMask 1.
-- SpellFamilyMask keys the abilities each clause names, using their own Spell.dbc family flags, OR-ing
-- every record of an ability that has more than one: Primordial Blast 4194304/1048576/64; Body of
-- Lightning 0/4194336/0; Warpdagger 0/8388608/0; Shock 6144/0/0 plus Gale 16384/537001984/136 plus
-- Electrocute 33554432/2097152/32 -> 33576960/539099136/168; Storm Ascendance 0/0/1.
DELETE FROM `spell_proc` WHERE `SpellId` IN (300944, 705686, 500250, 300827, 707618);
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(300944, 0, 38, 4194304, 1048576, 64, 69904, 1, 2, 0, 0, 0, 0, 0, 0, 0),
(705686, 0, 22, 0, 4194336, 0, 87312, 7, 1, 0, 0, 0, 0, 0, 0, 0),
(500250, 0, 38, 0, 8388608, 0, 87312, 7, 1, 0, 0, 0, 0, 0, 0, 0),
(300827, 0, 22, 33576960, 539099136, 168, 69904, 1, 1, 0, 0, 0, 0, 0, 0, 0),
(707618, 0, 22, 0, 0, 1, 87312, 7, 1, 0, 0, 0, 0, 0, 0, 0);
