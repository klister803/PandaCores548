INSERT INTO `spell_target_filter` (`spellId`, `targetId`, `count`, `comments`) VALUES ('117640', '30', '6', 'Spinning Crane Kick');
INSERT INTO `spell_target_filter` (`spellId`, `targetId`, `option`, `param1`, `comments`) VALUES ('117640', '30', '3', '16', 'Spinning Crane Kick');
INSERT INTO `spell_target_filter` (`spellId`, `targetId`, `option`, `comments`) VALUES ('117640', '30', '16', 'Spinning Crane Kick');

DELETE FROM `spell_proc_event` WHERE `entry` IN (146199);
INSERT INTO `spell_proc_event` VALUES (146199, 0, 0, 0, 0, 0, 0, 16384, 0, 0, 0, 0, 7);

DELETE FROM `spell_linked_spell` WHERE (`spell_trigger`='53563') AND (`spell_effect`='53651') AND (`type`='2') AND (`hastalent`='0') AND (`actiontype`='0');
INSERT INTO `spell_linked_spell` (`spell_trigger`, `spell_effect`, `target`, `comment`) VALUES ('-53563', '-53651', '3', 'Beacon of Light');
DELETE FROM `spell_proc_event` WHERE (`entry`='53651') AND (`effectmask`='7');