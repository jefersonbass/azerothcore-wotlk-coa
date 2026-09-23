-- On The Hunt (804708), issue #3430: "Your critical strikes with ranged abilities have a bonus effect
-- depending on your Ranged Weapon. With a Bow, ranged critical attacks generate an additional Archery
-- Point. With a Crossbow, they deal an additional $804714s1 damage and regenerate $804714s2% of your
-- maximum Focus."
--
-- Measured before writing - the whole chain, because the shape of the fix depends on where it stops:
--   * 804708 carries a live SPELL_AURA_PROC_TRIGGER_SPELL (aura 42) on 802153, but its Spell.dbc
--     ProcFlags are 0 and no spell_proc row supplied them, so the proc has never fired.
--   * 802153 is the end of the chain: a single effect 3 (DUMMY), enemy target, with NO trigger spell and
--     NO tooltip. The record therefore wires no payload at all - the two payloads the tooltip promises
--     exist only in its text. So this is a script, not a row plus a counter: the 804708 base points
--     [0, 29, 39] are vestigial (the record has one effect, so effects 1 and 2 do not exist).
--   * 804714 Heavy Bolt exists (effect 2 plus 137, "Deals $s1 damage and regenerates $s2 Focus") and is
--     the Crossbow payload; "Archery Point" is not a spell at all - it is the module's Advantage
--     resource.
--   * The row mirrors 524831, the closest Ranger precedent, field for field: ProcFlags 256
--     (DONE_SPELL_RANGED_DMG_CLASS - ranged ability damage, which is what "ranged abilities" means),
--     SpellTypeMask 1 (DAMAGE), SpellPhaseMask 2 (HIT) and HitMask 2 (PROC_HIT_CRITICAL, the
--     "critical strikes" half). SpellFamilyName 27 with mask 0 lets the script's own DmgClass test
--     narrow it further.
--   * The script half reuses what the module already has: the ranged test is
--     spellInfo->DmgClass == SPELL_DAMAGE_CLASS_RANGED (the guard HandleRangerQuiverHit already opens
--     with at AscensionClassMechanics.cpp:1068), the Bow branch is AddRangerAdvantage(player, 1) - the
--     Advantage resource, AscensionClassMechanics.cpp:997 - taken in the exact shape of the Archery
--     Master block above it (:1529-1530, crit plus HasAura plus TryMarkScriptEventHandled), and the
--     Crossbow branch is a plain cast of 804714. No new machinery.
DELETE FROM `spell_proc` WHERE `SpellId` = 804708;
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(804708, 0, 27, 0, 0, 0, 256, 1, 2, 2, 0, 0, 0, 0, 0, 0);
