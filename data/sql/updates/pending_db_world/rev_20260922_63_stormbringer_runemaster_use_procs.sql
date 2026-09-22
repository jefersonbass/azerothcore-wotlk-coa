-- Stormbringer and Runemaster talents whose proc auras ship without proc flags (Spell.dbc ProcFlags 0
-- and no `spell_proc` row), so the clauses below never fired. Payloads are already authored and native:
-- the aura-42 handler casts each record's TriggerSpell (Electrocutioner 804592, Wind's Grace 680861,
-- Primordial Studies 524982). Chance stays 0 everywhere, deferring to each record's own ProcChance
-- (all 100).
-- Aether Focus and Wind's Grace read "Your <ability> now grants ...", and those abilities can be buffs,
-- so they use ProcFlags 87312 (any spell carrying a damage class, positive or negative) with
-- SpellTypeMask 7 and SpellPhaseMask 1 - the cast-phase shape for self-buff clauses, as in
-- rev_1789944986203842691. Primordial Studies is bound to Primordial Blast's damage, so it uses 69904
-- with SpellTypeMask 1 and the hit phase.
-- SpellFamilyName is set on every row: SpellInfo::IsAffected returns true as soon as the family name is
-- 0, so a mask without its family is never consulted. 22 is Stormbringer, 38 is Runemaster.
-- SpellFamilyMask keys the ability each clause names, OR-ing every record of an ability that has more
-- than one: Skyfall 262144/0/0; Body of Lightning 0/32/0 OR 0/4194304/0 -> 0/4194336/0 plus Cloudburst
-- 0/0/8388608; Primordial Blast 4194304/1048576/64.
-- Primordial Studies also carries a native effect-1 spellmod; only the cooldown clause rides this row.
DELETE FROM `spell_proc` WHERE `SpellId` IN (705713, 705664, 802157);
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(705713, 0, 22, 262144, 0, 0, 87312, 7, 1, 0, 0, 0, 0, 0, 0, 0),
(705664, 0, 22, 0, 4194336, 8388608, 87312, 7, 1, 0, 0, 0, 0, 0, 0, 0),
(802157, 0, 38, 4194304, 1048576, 64, 69904, 1, 2, 0, 0, 0, 0, 0, 0, 0);
