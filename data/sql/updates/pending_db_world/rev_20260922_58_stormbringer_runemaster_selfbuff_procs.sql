-- Stormbringer and Runemaster talents whose proc auras ship without proc flags (Spell.dbc ProcFlags 0
-- and no `spell_proc` row), so the clauses below never fired. Payloads are already authored and native:
-- the aura-42 handler casts each record's TriggerSpell (Runebound Surge 707426, Prismatic Flow 705582,
-- Hurricanes 570129, Focal Point 680876, Sky and Stone 653228). Chance stays 0 everywhere, deferring to
-- each record's own ProcChance (all 100 except Sky and Stone's 10).
-- Clauses that fire on casting a self-buff (Runebound Surge, Prismatic Flow, Hurricanes) use ProcFlags
-- 87312 - any spell that carries a damage class, positive or negative - with SpellTypeMask 7 and
-- SpellPhaseMask 1 (PROC_SPELL_PHASE_CAST), the shape already used for cast-phase rows such as
-- rev_1789944986203842691. Clauses that name a damaging ability (Focal Point on Call Lightning,
-- Sky and Stone on the Weapon Engravings) use ProcFlags 69904 (the four direct damage spell classes),
-- SpellTypeMask 1 and the hit phase.
-- SpellFamilyMask keys the abilities each clause names, using their own Spell.dbc family flags:
-- Runeshroud 32768/1073741824/134217728 (both records OR-ed), Phase Out 64/0/0, Unshackle 16/0/0,
-- Call Lightning 0/16/32, Weapon Engraving: Air 131072/2147745792/0 OR Weapon Engraving: Earth
-- 131072/131072/0 -> 131072/2147876864/0.
DELETE FROM `spell_proc` WHERE `SpellId` IN (707423, 705581, 300839, 706629, 560053);
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(707423, 0, 38, 32768, 1073741824, 134217728, 87312, 7, 1, 0, 0, 0, 0, 0, 0, 0),
(705581, 0, 38, 64, 0, 0, 87312, 7, 1, 0, 0, 0, 0, 0, 0, 0),
(300839, 0, 22, 16, 0, 0, 87312, 7, 1, 0, 0, 0, 0, 0, 0, 0),
(706629, 0, 22, 0, 16, 32, 69904, 1, 1, 0, 0, 0, 0, 0, 0, 0),
(560053, 0, 38, 131072, 2147876864, 0, 69904, 1, 2, 0, 0, 0, 0, 0, 0, 0);
