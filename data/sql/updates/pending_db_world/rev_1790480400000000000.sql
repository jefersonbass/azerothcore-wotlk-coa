-- Both Seals stack on one recipient (#5798): Seal of Alysrazor and Seal of Al'ar can
-- sit on the same character, and each normal seal stacks with its greater version.
-- Seal of Alysrazor ranks 1-5 share the chain 800196 -> 802819 -> 802820 -> 802821
-- -> 802822 (spell_ranks, rev_20260831_01) with unranked Greater Seal of Alysrazor
-- 570170 (same +31 value as rank 5, raid version); Seal of Al'ar ranks 1-5 share the
-- chain 803649 -> 803729 -> 807704 -> 807770 -> 808012 (spell_ranks, rev_20260903_01)
-- with unranked Greater Seal of Al'ar 808060. No spell_group listed the Alysrazor
-- pair, so Aura::CanStackWith let both stay, and nothing linked the two seals to each
-- other. Group 1145 with rule 2 (SPELL_GROUP_STACK_RULE_EXCLUSIVE_FROM_SAME_CASTER)
-- makes the newest seal replace the caster's previous one while a second Pyromancer's
-- still coexists, the same shape as the Seal of Al'ar (1144), Starcaller Celestial
-- Mind (1142) and Arcane Protection (1143) groups. Only the first rank of each chain
-- is listed because SpellMgr::LoadSpellGroups drops any other rank and chains resolve
-- through GetFirstSpellInChain (src/server/game/Spells/SpellMgr.cpp).
START TRANSACTION;
DELETE FROM `spell_group` WHERE `id` = 1145;
INSERT INTO `spell_group` (`id`, `spell_id`) VALUES
(1145, 800196),
(1145, 570170),
(1145, 803649),
(1145, 808060);
DELETE FROM `spell_group_stack_rules` WHERE `group_id` = 1145;
INSERT INTO `spell_group_stack_rules` (`group_id`, `stack_rule`, `description`) VALUES
(1145, 2, 'Local Pyromancer: one Seal per caster on each recipient');
COMMIT;
