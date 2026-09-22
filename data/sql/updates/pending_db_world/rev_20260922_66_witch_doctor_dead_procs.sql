-- Witch Doctor batch, same shape as rev_20260922_63: an aura-42 passive with Spell.dbc ProcFlags 0 and no
-- `spell_proc` row, so the clause never fired while the payload stays authored (the aura-42 handler casts the
-- record's own TriggerSpell). Every payload is family 19 like the talent (no cross-class trigger).
-- Chance stays 0 everywhere, so LoadSpellProcs falls back to the record's own ProcChance (Sen'jin's Disciple
-- 25%, the others 100%); Sen'jin's Disciple's "once every 6 sec" is the Cooldown column.
-- Quick Voodoo (705854): "Direct critical healing done" - ProcFlags 2560 = PROC_FLAG_DONE_SPELL_NONE_DMG_CLASS_POS
-- | PROC_FLAG_DONE_SPELL_MAGIC_DMG_CLASS_POS (the beneficial spell pair, which is what a heal raises),
-- SpellTypeMask 2 (heal) and HitMask 2 = PROC_HIT_CRITICAL.
-- Sen'jin's Disciple (705863): "Direct heals now have a 25% chance" - same beneficial pair with SpellTypeMask 2
-- (heal) and no hit restriction.
-- Da True Voodoo Shuffle (705853) and Brew Lobber (705868) are "after using X" clauses, so SpellPhaseMask 1
-- = PROC_SPELL_PHASE_CAST with SpellTypeMask 7 (all types; an elixir or cauldron cast is beneficial, not a
-- damaging event). Their named abilities carry no isolatable family bit - Allcure Elixir 804049 has zero
-- FamilyFlags and Voodoo Cauldron 804684 shares flags[1] 0x1000000 with Voodoo Ward 500013/501075/504821/572841
-- - so both rows stay family 0 with no mask and aura_ascension_spell_list_talent_proc filters the event spell
-- against AscensionSpellListTalentProcs.h.
DELETE FROM `spell_proc` WHERE `SpellId` IN (705853, 705854, 705863, 705868);
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(705853, 0, 0, 0, 0, 0, 72464, 7, 1, 0, 0, 0, 0, 0, 0, 0),
(705854, 0, 0, 0, 0, 0, 2560, 2, 2, 2, 0, 0, 0, 0, 0, 0),
(705863, 0, 0, 0, 0, 0, 2560, 2, 2, 0, 0, 0, 0, 0, 6, 0),
(705868, 0, 0, 0, 0, 0, 72464, 7, 1, 0, 0, 0, 0, 0, 0, 0);

DELETE FROM `spell_script_names` WHERE `ScriptName` = 'spell_ascension_spell_list_talent_proc' AND `spell_id` IN (705853, 705868);
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(705853, 'spell_ascension_spell_list_talent_proc'),
(705868, 'spell_ascension_spell_list_talent_proc');
