INSERT INTO `spell_target_filter` (`spellId`, `targetId`, `count`, `comments`) VALUES ('117640', '30', '6', 'Spinning Crane Kick');
INSERT INTO `spell_target_filter` (`spellId`, `targetId`, `option`, `param1`, `comments`) VALUES ('117640', '30', '3', '16', 'Spinning Crane Kick');
INSERT INTO `spell_target_filter` (`spellId`, `targetId`, `option`, `comments`) VALUES ('117640', '30', '16', 'Spinning Crane Kick');

DELETE FROM `spell_proc_event` WHERE `entry` IN (146199);
INSERT INTO `spell_proc_event` VALUES (146199, 0, 0, 0, 0, 0, 0, 16384, 0, 0, 0, 0, 7);