-- Proc rows for talents whose named abilities cannot be isolated by a spell-class mask. IsAffected
-- (SpellInfo.cpp:1439) matches on ANY shared family bit, so a mask built from Quickdraw, Dawn Blade, Dusk
-- Blade or Primordial Blast also selects Grasp of the Undying 680483, Sixfold Shot 807527, Surging Blade
-- 681788, Witchblood Fever 684330, Dark Peril 685020, 686020, Hydros 713002, Plasma Ball 802966 and Runic
-- Obliteration 807014/807025 (measured with mask_scan.py).
-- The rows therefore carry family 0 and no mask; aura_ascension_spell_list_talent_proc filters the event
-- spell against each talent's own list in AscensionSpellListTalentProcs.h.
-- Bane of Witches (705451): Quickdraw silences the target.
-- Dusk and Dawn (705483): Dawn Blade and Dusk Blade damage reduces Darkslayer's Lantern's cooldown.
-- Magic Etchings (300582): Primordial Blast damage applies Leybreaker's armour reduction.
DELETE FROM `spell_proc` WHERE `SpellId` IN (705451, 705483, 300582);
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(705451, 0, 0, 0, 0, 0, 69904, 1, 2, 0, 0, 0, 0, 0, 0, 0),
(705483, 0, 0, 0, 0, 0, 69904, 1, 2, 0, 0, 0, 0, 0, 0, 0),
(300582, 0, 0, 0, 0, 0, 69904, 1, 2, 0, 0, 0, 0, 0, 0, 0);

DELETE FROM `spell_script_names` WHERE `ScriptName` = 'spell_ascension_spell_list_talent_proc';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(705451, 'spell_ascension_spell_list_talent_proc'),
(705483, 'spell_ascension_spell_list_talent_proc'),
(300582, 'spell_ascension_spell_list_talent_proc');
