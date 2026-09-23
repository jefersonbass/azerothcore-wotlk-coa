-- Mechsuit appearance (#4120, #4797): 801384's effect 0 triggers 803451, whose effect 0 is aura 78
-- (SPELL_AURA_MOUNTED) with misc 229921, so the suit's look is a mount. A mounted player fails
-- Spell::CheckCast with SPELL_FAILED_NOT_MOUNTED for everything outside the family-34 carve-out at
-- Spell.cpp:6094, and any loss of the mount aura runs 803451's Remove hook, which ejects the player from
-- the suit (AscensionTinkerAuras.cpp:98). Gatling Gun (500213) and Laser Beam (805372) are both named in
-- that exit path.
-- 809000 carries the same model without the mount: effect 0 = 6 (SPELL_EFFECT_APPLY_AURA), aura 56
-- (SPELL_AURA_TRANSFORM), misc 229921. 229921 is the creature_template entry "Build:Mechsuit" (type 9),
-- whose single model row is display 916645, so HandleAuraTransform resolves the model the mount did -
-- through UNIT_FIELD_DISPLAYID instead of UNIT_FIELD_MOUNTDISPLAYID. DurationIndex 21 is infinite; the
-- module applies and removes it with the suit. 809000 is absent from the client Spell.dbc, so this row
-- creates the spell rather than replacing one.
DELETE FROM `spell_dbc` WHERE `ID` = 809000;
INSERT INTO `spell_dbc` (`ID`, `DurationIndex`, `Effect_1`, `ImplicitTargetA_1`, `EffectAura_1`, `EffectMiscValue_1`, `Name_Lang_enUS`) VALUES
(809000, 21, 6, 1, 56, 229921, 'Mechsuit Appearance');
