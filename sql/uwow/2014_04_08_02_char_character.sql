ALTER TABLE  `characters` CHANGE  `petslotused`  `petslot` LONGTEXT CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL;
UPDATE `characters` SET `petslot` = 0;

-- Warlock
update `characters` set `currentpetslot` = `currentpetslot` - 25 WHERE `currentpetslot` > 24 AND `currentpetslot` < 36;
-- Stamped
update `characters` set `currentpetslot` = `currentpetslot` - 36 WHERE `currentpetslot` > 35 AND `currentpetslot` < 41;
-- Other
update `characters` set `currentpetslot` = 56 WHERE `currentpetslot` = 100;

-- important lauch this ones at startup
UPDATE `worldstates` SET value = 0x20 WHERE entry = 20004;