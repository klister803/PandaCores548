DELETE FROM `spell_proc_check` WHERE (`entry`='56218') AND (`entry2`='0') AND (`entry3`='0') AND (`checkspell`='146739') AND (`hastalent`='0') AND (`powertype`='-1') AND (`dmgclass`='-1') AND (`specId`='0');
DELETE FROM `spell_proc_check` WHERE (`entry`='56218') AND (`entry2`='0') AND (`entry3`='0') AND (`checkspell`='348') AND (`hastalent`='0') AND (`powertype`='-1') AND (`dmgclass`='-1') AND (`specId`='0');

DELETE FROM `spell_proc_event` WHERE `entry` IN (56218);
INSERT INTO `spell_proc_event` VALUES (56218, 0, 0, 6, 0, 0, 0, 0, 0, 0, 100, 0, 7);