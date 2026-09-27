-- Seal of Al'ar / Greater Seal of Al'ar (#4883): casting the greater seal does not
-- remove the lesser seal. Seal of Al'ar ranks 1-5 share the chain
-- 803649 -> 803729 -> 807704 -> 807770 -> 808012 (spell_ranks, rev_20260903_01) and
-- Greater Seal of Al'ar 808060 (same +33 value as rank 5, raid version) is unranked,
-- and no spell_group listed them, so Aura::CanStackWith let both stay. Group 1144
-- with rule 2 (SPELL_GROUP_STACK_RULE_EXCLUSIVE_FROM_SAME_CASTER) makes the newest
-- replace the caster's previous one while a second Pyromancer's still coexists, the
-- same shape as the Starcaller Celestial Mind (1142), Arcane Protection (1143),
-- Templar Gift (1140) and Felsworn Intuition (1141) groups. Only the first rank is
-- listed because SpellMgr::LoadSpellGroups drops any other rank and chains resolve
-- through GetFirstSpellInChain (src/server/game/Spells/SpellMgr.cpp).
START TRANSACTION;
DELETE FROM `spell_group` WHERE `id` = 1144;
INSERT INTO `spell_group` (`id`, `spell_id`) VALUES
(1144, 803649),
(1144, 808060);
DELETE FROM `spell_group_stack_rules` WHERE `group_id` = 1144;
INSERT INTO `spell_group_stack_rules` (`group_id`, `stack_rule`, `description`) VALUES
(1144, 2, 'Local Pyromancer: one Seal of Alar per caster on each recipient');
COMMIT;
