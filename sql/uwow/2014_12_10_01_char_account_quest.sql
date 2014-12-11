ALTER TABLE `character_queststatus`
ADD `account` INT(11) DEFAULT '0' NOT NULL AFTER guid,;
ALTER TABLE `character_queststatus`
ADD KEY `account` (`account`);
ALTER TABLE `character_queststatus_daily`
ADD `account` INT(11) DEFAULT '0' NOT NULL AFTER guid;
ALTER TABLE `character_queststatus_daily`
ADD KEY `account` (`account`);
ALTER TABLE `character_queststatus_rewarded`
ADD `account` INT(11) DEFAULT '0' NOT NULL AFTER guid;
ALTER TABLE `character_queststatus_rewarded`
ADD KEY `account` (`account`);
ALTER TABLE `character_queststatus_seasonal`
ADD `account` INT(11) DEFAULT '0' NOT NULL AFTER guid;
ALTER TABLE `character_queststatus_seasonal`
ADD KEY `account` (`account`);
ALTER TABLE `character_queststatus_weekly`
ADD `account` INT(11) DEFAULT '0' NOT NULL AFTER guid;
ALTER TABLE `character_queststatus_weekly`
ADD KEY `account` (`account`);