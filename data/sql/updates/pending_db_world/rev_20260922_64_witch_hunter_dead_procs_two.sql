-- Second Witch Hunter batch, same shape as rev_20260922_63: an aura-42 passive with Spell.dbc ProcFlags 0 and
-- no `spell_proc` row, so the clause never fired while the payload stays authored (the aura-42 handler casts
-- the record's own TriggerSpell).
-- Hired Crossbow (705501): "Critical strikes reduce the remaining cooldown of Set Bounty" - ProcFlags 69972
-- (melee and ranged auto attacks plus the four direct damage spell classes, the reviewed shape for a crit
-- clause) with HitMask 2 = PROC_HIT_CRITICAL. The record's SpellFamilyName is 38 (Runemaster) but the talent
-- is taught by SkillLine 58 "Boltslinger" (Witch Hunter), so the family here is 21 - a family-38 row would
-- never match the abilities that can actually trigger it.
-- Darkstalker (500091): "your auto attacks reduce its remaining cooldown by 1 sec" - ProcFlags 68 =
-- PROC_FLAG_DONE_MELEE_AUTO_ATTACK | PROC_FLAG_DONE_RANGED_AUTO_ATTACK, outside the spell and phase masks.
-- Cursewarding (804688): "Dealing or taking Shadow Damage has a 10% chance to grant you Dark Embrace" -
-- ProcFlags 1118548 = 69972 (direct damage dealt) | 1048576 (PROC_FLAG_TAKEN_DAMAGE), with SchoolMask 32
-- (shadow) so only shadow damage counts. Chance 0 defers to the record's ProcChance = 10.
DELETE FROM `spell_proc` WHERE `SpellId` IN (500091, 705501, 804688);
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(500091, 0, 21, 0, 0, 0, 68, 0, 0, 0, 0, 0, 0, 0, 0, 0),
(705501, 0, 21, 0, 0, 0, 69972, 1, 2, 2, 0, 0, 0, 0, 0, 0),
(804688, 32, 0, 0, 0, 0, 1118548, 1, 2, 0, 0, 0, 0, 0, 0, 0);
