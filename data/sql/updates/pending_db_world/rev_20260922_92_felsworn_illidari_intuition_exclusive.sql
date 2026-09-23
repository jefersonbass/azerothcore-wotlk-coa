-- Felsworn Illidari Intuition (#4718): one Illidari Intuition per caster on each recipient. The
-- single-target version and its raid version are separate spells - 800212 is the first rank of the
-- chain (spell_ranks: 800212 -> 501326 -> 501327 -> 501328 -> 501329) and 680308 "Greater Illidari
-- Intuition" is unranked - and no spell_group listed them, so Aura::CanStackWith let both stay. Rule 2
-- (SPELL_GROUP_STACK_RULE_EXCLUSIVE_FROM_SAME_CASTER) makes the newest replace the caster's previous
-- one while a second Felsworn's still coexists, the same choice recorded for the Witch Hunter Edicts,
-- Venomancer Pheromones, Chronomancer Wisdom and Templar Gift groups. Only the first rank is listed
-- because SpellMgr::LoadSpellGroups drops any other rank and
-- SpellMgr::GetSpellSpellGroupMapBounds resolves chains through GetFirstSpellInChain.
DELETE FROM `spell_group` WHERE `id` = 1141;
INSERT INTO `spell_group` (`id`, `spell_id`) VALUES
(1141, 800212),
(1141, 680308);
DELETE FROM `spell_group_stack_rules` WHERE `group_id` = 1141;
INSERT INTO `spell_group_stack_rules` (`group_id`, `stack_rule`, `description`) VALUES
(1141, 2, 'Local Felsworn: one active Illidari Intuition per caster');
