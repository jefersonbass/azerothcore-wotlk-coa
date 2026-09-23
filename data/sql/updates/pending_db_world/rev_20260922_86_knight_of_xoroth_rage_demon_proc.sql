-- CoA Rage Demon (500313): "Your direct damage dealt with abilities now has a 10% chance to generate
-- an additional stack of Demonfire and Demon's Blood". The talent's proc never fired.
--
-- Measured before writing:
--   * 500313 carries two SPELL_AURA_PROC_TRIGGER_SPELL effects (aura 42) with triggers 524912 "Add 1
--     Demonfire" and 800999 "Demon's Blood", both targeting the caster, plus a third effect with aura
--     220 - which is not an aura type in this client's enum (220 is an animation id and a skill line),
--     so it is an unhandled custom aura and does not affect the other two.
--   * ProcChance is 10, so by this class's discriminator the aura is a real chance proc rather than a
--     marker, and the row's Chance stays 0 so the loader falls back to that 10.
--   * Its Spell.dbc ProcFlags are 0 and there was no spell_proc row, so neither effect could fire.
--     The CoA world package has no row for it either, and that table IS covered by the package (1245
--     rows), so the gap is inherited rather than lost.
--   * ProcFlags 69904 = DONE_SPELL_MELEE_DMG_CLASS | DONE_SPELL_RANGED_DMG_CLASS |
--     DONE_SPELL_NONE_DMG_CLASS_NEG | DONE_SPELL_MAGIC_DMG_CLASS_NEG - damage dealt by abilities, with
--     no auto-attack bits. That is exactly the tooltip's "direct damage dealt with abilities"; the
--     neighbouring set 65876 would have added melee and ranged auto-attacks.
--   * SpellTypeMask 1 (DAMAGE) and SpellPhaseMask 2 (HIT) complete the shape, matching 520138.
--   * No aura script and no Rules entry are needed: both triggers target the caster, so the default
--     aura-42 action casts them, and the condition is any ability damage rather than a spell list.
--
-- Note: this is the first spell_proc row carrying SpellFamilyName 23 (Knight of Xoroth) - the table had
-- none. The family is taken from the spell itself; SpellFamilyMask stays 0 because the row is not
-- restricted to a spell group.
DELETE FROM `spell_proc` WHERE `SpellId` = 500313;
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(500313, 0, 23, 0, 0, 0, 69904, 1, 2, 0, 0, 0, 0, 0, 0, 0);
