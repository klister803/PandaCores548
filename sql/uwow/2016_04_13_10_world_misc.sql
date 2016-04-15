DELETE FROM `spell_proc_event` WHERE `entry` IN (44448);
INSERT INTO `spell_proc_event` VALUES (44448, 4, 3, 4194304+1+16, 67108864+4096, 2097152, 0, 0, 0x00000C03, 0, 0, 0, 7);

INSERT INTO `spell_linked_spell` (`spell_trigger`, `spell_effect`, `hitmask`, `comment`) VALUES ('33876', '34071', '1', 'Mangle combo point');

DELETE FROM `spell_proc_event` WHERE `entry` IN (14190);
INSERT INTO `spell_proc_event` VALUES (14190, 0, 8, 512, 6, 256, 0, 0, 2, 0, 0, 0, 7);

DELETE FROM `spell_proc_event` WHERE `entry` IN (13877);
INSERT INTO `spell_proc_event` VALUES (13877, 0, 8, 2+131072+536870912+512, 1+8388608+268435456, 2+2048+65536+32768, 0, 20, 2147483648, 0, 100, 0, 7);