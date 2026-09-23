-- Six Stormbringer talent gates that are dead in the DBC: each carries an aura 42 with ProcFlags 0, and
-- neither an explicit row in `spell_proc` nor module code covers it (the three-link test: row? module? then
-- ProcFlags 0 closes). Same defect and same shape as rev_20260922_70/72/93/94.
--
-- Verbs come from each tooltip, and the ProcFlags sets are the ones already recorded for this fork:
--   87312 = PROC_FLAG_DONE_SPELL_MELEE_DMG_CLASS | DONE_SPELL_RANGED_DMG_CLASS |
--           DONE_SPELL_NONE_DMG_CLASS_POS | DONE_SPELL_NONE_DMG_CLASS_NEG |
--           DONE_SPELL_MAGIC_DMG_CLASS_POS | DONE_SPELL_MAGIC_DMG_CLASS_NEG  (cast clause)
--   69904 = the same set without the two POS classes                                (damage clause)
--
-- #1360 Storm Synergy 578300: "Your Electrocute can now be used while channeling Stormflow and causes it
--   to apply Conductive." Cast clause.
-- #1692 Static Electricity 524954: "Critical strikes with Torrential Wrath now spawn an Electrified Water
--   Elemental to aid you in combat." Critical clause -> HitMask 2 (PROC_HIT_CRITICAL, SpellMgr.h:258).
-- #1811 Sparks 301298: "Damage dealt by Electrocute and Arm of Thorim now increase the target's chance to
--   be critically hit by spells." Damage clause.
-- #3410 Thunder King 804591: "For $d, your Call Lightning strikes $s3 additional nearby [targets]." Cast
--   clause. This record also ships without SPELL_ATTR0_PASSIVE, so AscensionStormbringerContracts.cpp marks
--   it passive - without that the aura never applies and this row would have nothing to gate.
-- #3637 Amped Flow 806411: "While channeling Stormflow, your Shock now strikes an additional target." Cast
--   clause.
-- #3667 Critical Circuit 807314: "Direct critical strikes with Static spenders now refund 10 Static."
--   Critical clause -> HitMask 2.
DELETE FROM `spell_proc` WHERE `SpellId` IN (578300, 524954, 301298, 804591, 806411, 807314);
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(578300, 0, 0, 0, 0, 0, 87312, 7, 1, 0, 0, 0, 0, 0, 0, 0),
(524954, 0, 0, 0, 0, 0, 69904, 1, 2, 2, 0, 0, 0, 0, 0, 0),
(301298, 0, 0, 0, 0, 0, 69904, 1, 2, 0, 0, 0, 0, 0, 0, 0),
(804591, 0, 0, 0, 0, 0, 87312, 7, 1, 0, 0, 0, 0, 0, 0, 0),
(806411, 0, 0, 0, 0, 0, 87312, 7, 1, 0, 0, 0, 0, 0, 0, 0),
(807314, 0, 0, 0, 0, 0, 69904, 1, 2, 2, 0, 0, 0, 0, 0, 0);
