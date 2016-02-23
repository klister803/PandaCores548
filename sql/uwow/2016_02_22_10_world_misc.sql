DELETE FROM `spell_proc_event` WHERE `entry` IN (44448);
INSERT INTO `spell_proc_event` VALUES (44448, 4, 3, 0, 0, 0, 0, 0, 0x00000C02, 0, 0, 0, 7);

DELETE FROM `spell_proc_event` WHERE `entry` IN (119032);
INSERT INTO `spell_proc_event` VALUES (119032, 0, 0, 0, 0, 0, 0, 0x000A22A8, 0, 0, 0, 0, 7);

UPDATE `areatrigger_actions` SET `aura`='-116014' WHERE (`entry`='304') AND (`id`='0');