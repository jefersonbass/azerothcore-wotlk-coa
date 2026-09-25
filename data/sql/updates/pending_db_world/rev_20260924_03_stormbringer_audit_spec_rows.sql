-- Restores three Stormbringer proc rows to the shape their own gameplay scenarios assert. The rows
-- were written by the 2026-09-23 audit batch (rev_20260922_03, rev_20260922_14, rev_20260922_19) and
-- then overwritten by rev_20260922_56 / rev_20260922_60, which sort later: the updater orders files by
-- name (DBUpdater.cpp:485), so a higher suffix applies last. This file sorts after all of them, so the
-- restore reaches both a database that applies everything fresh and one that already ran the pair.
--
-- THE BASIS IS THE SCENARIO ASSERTION, NOT AN INDEPENDENT MEASUREMENT:
--   705700  apps/coa-gameplay-test/scenarios/stormbringer-fix-invigoration-stack-family.json
--           names the trigger: "the Aeroblast hit procs aura 705700" -> SpellPhaseMask 2 (HIT). The
--           overwriting row used phase 1 (CAST), so the hit clause could not fire.
--   705715  apps/coa-gameplay-test/scenarios/stormbringer-fix-air-elemental-family-gift-of-air.json
--           asserts "the critical strike adds 583254's 1500 ms to Tailwind"; a critical needs the HIT
--           phase, so the row needs SpellPhaseMask 3 (CAST|HIT) with HitMask 3. The overwriting row
--           used phase 1 only, so the crit clause could not fire.
--   705719  apps/coa-gameplay-test/scenarios/stormbringer-fix-on-cast-proc-cooldowns-master-airbender.json
--           asserts a negative - "Arm of Thorim is outside Gale's 0x4000 mask" - so the row must carry
--           Gale's 0x4000 alone; the overwriting row added SpellFamilyMask1 536870912 (0x20000000)
--           and SpellFamilyMask2 128 (0x80) on top of it.
-- 705715 keeps the audit's five-column shape on purpose (no family filter): the crit clause is not
-- bound to one ability, and aura_ascension_gift_of_air, which this file does not touch, separates the
-- two clauses. The other two rows carry the audit's full column list.
DELETE FROM `spell_proc` WHERE `SpellId` IN (705700, 705715, 705719);
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(705700, 0, 22, 8388608, 0, 0, 65536, 1, 2, 0, 0, 2, 0, 100, 0, 0),
(705719, 0, 22, 16384, 0, 0, 65536, 0, 1, 0, 0, 0, 0, 100, 0, 0);

DELETE FROM `spell_proc` WHERE `SpellId` = 705715;
INSERT INTO `spell_proc` (`SpellId`, `ProcFlags`, `SpellPhaseMask`, `HitMask`, `Chance`) VALUES
(705715, 333140, 3, 3, 100);
