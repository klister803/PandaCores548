ALTER TABLE `character_achievement_progress`   
  ADD COLUMN `achievID` INT(11) UNSIGNED DEFAULT 0  NOT NULL AFTER `date`;
ALTER TABLE `account_achievement_progress`   
  ADD COLUMN `achievID` INT(11) UNSIGNED DEFAULT 0  NOT NULL AFTER `date`;
ALTER TABLE `guild_achievement_progress`   
  ADD COLUMN `achievID` INT(0) UNSIGNED NOT NULL AFTER `completedGuid`;