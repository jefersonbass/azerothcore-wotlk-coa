-- #4245 Conductive 500005: "Brine now grants Conductive for 30 sec, stacking 10 times. Torrential Wrath
-- consumes all stacks of Conductive to trigger Conduction." The record is passive and already carries the
-- PROC_TRIGGER_SPELL aura (effect 0: aura 42 -> 567559 Conductive), but Spell.dbc gives it ProcFlags 0, so
-- SpellMgr::LoadSpellProcs builds no entry for it and the aura can never fire - the same dead-gate shape as
-- rev_20260923_99. The second sentence of the tooltip is already implemented in
-- AscensionStormbringerTalents.cpp (Torrential Wrath removes the Static aura and triggers Conduction per
-- stack), so only the Brine half is missing.
--
-- ProcFlags 65536 = PROC_FLAG_DONE_SPELL_MAGIC_DMG_CLASS_NEG (SpellMgr.h:134), the done-spell class Brine
-- reports (Frost damage, magic damage class). SpellPhaseMask 1 = cast, so the grant lands once per cast
-- instead of once per target hit.
--
-- The clause names Brine and nothing else, so the row is filtered to it: SpellFamilyName 22 with
-- SpellFamilyMask1 128, the flags[1] value shared by all seven Brine ranks (807105-807111). Same
-- family-plus-mask form rev_20260922_14 uses for its on-cast talents.
DELETE FROM `spell_proc` WHERE `SpellId` = 500005;
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(500005, 0, 22, 0, 128, 0, 65536, 0, 1, 0, 0, 0, 0, 100, 0, 0);
