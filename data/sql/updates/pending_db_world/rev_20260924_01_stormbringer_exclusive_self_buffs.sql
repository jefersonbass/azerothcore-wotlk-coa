-- The Stormbringer's Pressure, Call and Aegis families are explicit alternative self buffs: each one's own
-- tooltip states the exclusivity ("Only 1 Pressure spell can be active at a time", "Does not stack with Call
-- of the Wind", "Can only have 1 Aegis up at a time"), but no group ever carried them, so the fork applied
-- every member at once and the player kept all the benefits (#4865, #4891).
--
-- Same shape and same stack rule as rev_20260907_02: rule 2, exclusive from the same caster - a second player
-- may still place their own Call on a different target, which rule 1 (global exclusive) would forbid.
-- Triggered helpers are not group members: the Call of the Wind pet heal (520088) is effect 136 HEAL, not an
-- aura, and the Pressure slow the AuraScript applies (803566) is not cast by the player.
START TRANSACTION;
DELETE FROM `spell_group` WHERE `id` IN (1133, 1134, 1135);
INSERT INTO `spell_group` (`id`, `spell_id`) VALUES
(1133, 803563),
(1133, 803564),
(1133, 803565),
(1134, 578311),
(1134, 578312),
(1134, 578313),
(1134, 578314),
(1134, 578315),
(1134, 578316),
(1134, 503319),
(1134, 503320),
(1134, 503321),
(1134, 503322),
(1134, 503323),
(1134, 680291),
(1134, 804018),
(1135, 680282),
(1135, 680590),
(1135, 680591),
(1135, 680592),
(1135, 680593),
(1135, 680594),
(1135, 680316),
(1135, 680334);
DELETE FROM `spell_group_stack_rules` WHERE `group_id` IN (1133, 1134, 1135);
INSERT INTO `spell_group_stack_rules` (`group_id`, `stack_rule`, `description`) VALUES
(1133, 2, 'Local Stormbringer: one active Pressure per caster'),
(1134, 2, 'Local Stormbringer: one active Call per caster'),
(1135, 2, 'Local Stormbringer: one active Aegis per caster');
COMMIT;
