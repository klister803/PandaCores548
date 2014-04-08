ALTER TABLE  `characters` CHANGE  `petslotused`  `petslot` LONGTEXT CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL;
UPDATE `characters` SET `petslot` = 0;

-- important lauch this ones at startup
UPDATE `worldstates` SET value = 0x20 WHERE entry = 20004;