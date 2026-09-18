-- Issue #462: Runemaster "Genesis" (500501) — runic brand accumulating 50% of
-- the Runemaster's damage dealt for 8 sec, unleashed as the banked amount when
-- the brand ends. The payout spell resolves the damage here (base point 0 is
-- supplied by the accumulator in AscensionRunemasterBrand.cpp).
DELETE FROM `spell_bonus_data` WHERE `entry` = 500502;
INSERT INTO `spell_bonus_data` (`entry`, `direct_bonus`, `dot_bonus`, `ap_bonus`, `ap_dot_bonus`, `comments`) VALUES
(500502, 0, 0, 0, 0, 'CoA Runemaster - Genesis payout (banked damage via SPELLVALUE_BASE_POINT0)');
