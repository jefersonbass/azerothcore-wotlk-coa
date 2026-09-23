-- Shallow Grave (128308, 128403): make the authored despawn actually run.
--
-- Reported as infinite loot of the Zul'Farrak Shallow Graves: the grave stays lootable and keeps
-- summoning Zul'Farrak zombies instead of disappearing once it has been looted.
--
-- Measured before writing:
--   * Both entries are the same object: type 3 CHEST, lootId 8367, consumable 1, artKit 0, and 40 spawns
--     in Zul'Farrak (map 209) - 31 of entry 128403 and 9 of entry 128308. Their gameobject_template and
--     smart_scripts rows are identical to the CoA world package, so nothing was lost in this fork.
--   * Both carry the same SmartAI summon: SMART_ACTION_CAST (11) of 10247 "Summon Zul'Farrak Zombies"
--     on SMART_EVENT_GOSSIP_HELLO (64), which GameObject::Use() fires on every open. That row is
--     SMART_EVENT_FLAG_NOT_REPEATABLE | SMART_EVENT_FLAG_DONT_RESET (257), so it fires once per
--     gameobject instance - the summon itself is not what repeats.
--   * 128308 already carries the follow-up row "Shallow Grave - On Gossip Hello - Despawn"
--     (SMART_ACTION_FORCE_DESPAWN (41), 300000 ms) but with target_type 0 = SMART_TARGET_NONE.
--     SmartScript::GetTargets() pushes no target for SMART_TARGET_NONE, and SMART_ACTION_FORCE_DESPAWN
--     only iterates the resolved target list, so the row despawned nothing. It is the only one of the
--     1753 SMART_ACTION_FORCE_DESPAWN rows in the world database with target_type 0; 121 of the 145
--     gameobject-sourced rows use SMART_TARGET_SELF (1).
--   * 128403 has no despawn row at all, so the majority of the graves had no backstop even in intent.
--   * Consequence of the inert despawn: a partially looted grave never leaves GO_ACTIVATED, so
--     GameObject::SaveRespawnTime() is never reached for it and no respawn time is stored for the
--     instance. The next time Zul'Farrak loads, the grave is recreated in GO_READY, Player::SendLoot()
--     refills the loot table and the fresh SmartAI instance runs the summon again - which is the
--     repeatable loot and repeatable zombie spawns in the report.
--   * SMART_TARGET_SELF resolves to the gameobject itself in a gameobject script
--     (SmartScript::GetBaseObject() returns the GameObject when no unit is bound), so the existing
--     128308 row starts targeting the grave it was written for, and the matching row is added to 128403.
--
-- The 300000 ms timer and the summon stay as authored; the consumable=1 flag still despawns a fully
-- looted grave immediately, so this row is only the backstop for graves left partially looted.
UPDATE `smart_scripts` SET `target_type` = 1
WHERE `entryorguid` = 128308 AND `source_type` = 1 AND `id` = 1 AND `action_type` = 41;

DELETE FROM `smart_scripts` WHERE `entryorguid` = 128403 AND `source_type` = 1 AND `id` = 1;
INSERT INTO `smart_scripts`
(`entryorguid`, `source_type`, `id`, `link`, `event_type`, `event_phase_mask`, `event_chance`, `event_flags`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `event_param5`, `event_param6`, `action_type`, `action_param1`, `action_param2`, `action_param3`, `action_param4`, `action_param5`, `action_param6`, `target_type`, `target_param1`, `target_param2`, `target_param3`, `target_param4`, `target_x`, `target_y`, `target_z`, `target_o`, `comment`) VALUES
(128403, 1, 1, 0, 64, 0, 100, 0, 0, 0, 0, 0, 0, 0, 41, 300000, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 'Shallow Grave - On Gossip Hello - Despawn');
