-- Stormbringer and Runemaster talents whose proc auras ship without proc flags (Spell.dbc ProcFlags 0
-- and no `spell_proc` row), so the clauses below never fired. Every payload is already authored and
-- native: the aura-42 handler casts each record's TriggerSpell, so only the proc event is restored here.
-- Chance stays 0 everywhere: LoadSpellProcs falls back to each record's own ProcChance, which is the
-- tooltip's value (Runeslinger 5%, Conduit 60%, Comforting Winds 100%, Waterfall 100%).
-- ProcFlags 69904 = the four direct damage spell classes only, used where the clause is spell-bound
-- ("dealing Frost damage"); 69972 = those plus melee and ranged auto attacks, used for the clauses that
-- say damage without naming a spell ("direct damage dealt", "damage dealt") and, per
-- rev_20260921_40_ranger_dead_procs.sql, for a critical-strike clause. Both exclude periodic, so HoTs
-- and DoTs never qualify. HitMask 2 = PROC_HIT_CRITICAL for Waterfall's "dealing critical damage";
-- HitMask 0 = any hit elsewhere. SpellTypeMask 1 = PROC_SPELL_TYPE_DAMAGE; SpellPhaseMask 2 =
-- PROC_SPELL_PHASE_HIT. Conduit (806374) additionally carries SchoolMask 16 (SPELL_SCHOOL_MASK_FROST)
-- for its "dealing Frost damage" clause.
-- Waterfall's payload 807661 is three effect-165 (SPELL_EFFECT_ASCENSION_MODIFY_COOLDOWN) rows cutting
-- 0.5 sec from 707365 / 500932 / 503352, so a critical hit trims those cooldowns natively.
DELETE FROM `spell_proc` WHERE `SpellId` IN (806374, 705550, 704209, 806412);
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(806374, 16, 0, 0, 0, 0, 69904, 1, 2, 0, 0, 0, 0, 0, 0, 0),
(705550, 0, 0, 0, 0, 0, 69972, 1, 2, 0, 0, 0, 0, 0, 0, 0),
(704209, 0, 0, 0, 0, 0, 69972, 1, 2, 0, 0, 0, 0, 0, 0, 0),
(806412, 0, 0, 0, 0, 0, 69972, 1, 2, 2, 0, 0, 0, 0, 0, 0);
