UPDATE `creature_template` SET `spell2`='0' WHERE (`entry`='5925');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES ('8178', 'spell_sha_grounding_totem');
UPDATE `spell_proc_event` SET `procFlags`='139264' WHERE (`entry`='8178') AND (`effectmask`='7');

DELETE FROM `spell_proc_event` WHERE (`entry`='974') AND (`effectmask`='7');
INSERT INTO `spell_proc_event` (`entry`, `procFlags`) VALUES ('974', '1048576');

INSERT INTO `spell_linked_spell` (`spell_trigger`, `spell_effect`, `hastalent`, `chance`, `hitmask`, `comment`) VALUES ('34026', '34720', '109306', '30', '1', 'Thrill of the Hunt');
INSERT INTO `spell_linked_spell` (`spell_trigger`, `spell_effect`, `hastalent`, `chance`, `hitmask`, `comment`) VALUES ('117050', '34720', '109306', '30', '1', 'Thrill of the Hunt');
INSERT INTO `spell_linked_spell` (`spell_trigger`, `spell_effect`, `hastalent`, `chance`, `hitmask`, `comment`) VALUES ('131894', '34720', '109306', '30', '1', 'Thrill of the Hunt');
INSERT INTO `spell_linked_spell` (`spell_trigger`, `spell_effect`, `hastalent`, `chance`, `hitmask`, `comment`) VALUES ('109259', '34720', '109306', '30', '1', 'Thrill of the Hunt');
INSERT INTO `spell_linked_spell` (`spell_trigger`, `spell_effect`, `hastalent`, `chance`, `hitmask`, `comment`) VALUES ('3674', '34720', '109306', '30', '1', 'Thrill of the Hunt');

DELETE FROM `spell_proc_event` WHERE `entry` IN (109306);
INSERT INTO `spell_proc_event` VALUES (109306, 0, 0, 2048+4096+131072, 2147483648, 16777216+256+1, 4+8192, 0, 0, 0, 0, 0, 7);