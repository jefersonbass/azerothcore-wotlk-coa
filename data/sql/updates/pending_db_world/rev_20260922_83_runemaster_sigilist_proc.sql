-- CoA Sigilist (705586): "After using a Palm Sigil, the duration of your crowd control effects is
-- increased by $706525s1%". The talent's proc never fired, so the buff was never granted.
--
-- Measured before writing:
--   * 705586 is SPELL_AURA_PROC_TRIGGER_SPELL (aura 42) with EffectTriggerSpell 706525, and it also
--     carries two native spellmods (aura 107 and 108) that already work - only the CC-duration half was
--     missing. ProcChance is 100, so by this class's discriminator the aura is a marker rather than a
--     chance proc; the event it needs is the Palm Sigil CAST.
--   * Its Spell.dbc ProcFlags are 0 and the world database had no spell_proc row, so the trigger never
--     fired.
--   * "Palm Sigil" is a family of 23 spells (Earth, Water, Fire, Arcane, Frost, Wind), and the tooltip
--     says "after using a Palm Sigil" without naming one, so the proc has to cover all 23.
--   * The class already implements "procs when the player casts one of these spells" through
--     AscensionStormbringerRunemasterTalentProcs, whose CheckProc matches the casting spell against the
--     rule's spell list. The Rules entry for 705586 carries the 23 Palm Sigil ids, so no per-sigil
--     registration is needed.
--   * SpellPhaseMask is 1 (CAST) rather than 2 (HIT) because the tooltip keys on USING the sigil. That
--     combination is established: 16 rows in the world carry ProcFlags 69904 with phase 1, four of them
--     family 38 like this one (520755, 706823, 806409, 806737), all with SpellFamilyMask 0 and the list
--     filtering done by the Rules table.
--
-- The existing aura_ascension_arcane_palm_sigil handler is not the right hook and is untouched: its
-- Check requires direct magic damage and its Proc detonates the sigil (DoT plus silence), which is the
-- detonation moment, not the cast the talent names.
DELETE FROM `spell_proc` WHERE `SpellId` = 705586;
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(705586, 0, 38, 0, 0, 0, 69904, 1, 1, 0, 0, 0, 0, 0, 0, 0);

DELETE FROM `spell_script_names` WHERE `spell_id` = 705586 AND `ScriptName` = 'spell_ascension_stormbringer_runemaster_talent_proc';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(705586, 'spell_ascension_stormbringer_runemaster_talent_proc');
