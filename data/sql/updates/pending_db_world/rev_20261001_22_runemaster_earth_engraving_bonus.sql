-- Runemaster Weapon Engraving: Earth tick 653272 (#6005): "15% chance to deal 35 Nature
-- to enemies within 8 yards" ticking for 3-4 at level 28.
--
-- Measured before writing (server-dbc/Spell.dbc + tools/ascension-ref exiles):
--   * 653272 effect 0 is SPELL_EFFECT_SCHOOL_DAMAGE with EffectBasePoints 12 (= 13 base,
--     the exiles "School Damage 13") and raw EffectBonusMultiplier 0.1.
--   * exiles "Scales with Attack Power (0.0875), Nature Power (0.115)".
--   * src/server/coa/AscensionStockCoefficients.cpp zeroes the multiplier for every id in
--     StockCoefficientSpells (653272 listed in AscensionStockCoefficientData.h), and no
--     spell_bonus_data row exists for 653272, so Unit::SpellDamageBonusDone
--     (src/server/game/Entities/Unit/Unit.cpp) adds no power term: 13 x ScalingBase(28)
--     = 13 x 0.308 ~= 4, exactly the reported 3-4. A present row overwrites the zeroed
--     multiplier (Unit.cpp: direct coeffs), same shape as Firebrand 653210 in
--     rev_20260927_91_runemaster_firebrand_detonation.sql.
DELETE FROM `spell_bonus_data` WHERE `entry` = 653272;
INSERT INTO `spell_bonus_data` (`entry`, `direct_bonus`, `dot_bonus`, `ap_bonus`, `ap_dot_bonus`, `comments`) VALUES
(653272, 0.115, 0, 0.0875, 0, 'CoA Earth Engraving - Weapon Engraving: Earth scaling');
