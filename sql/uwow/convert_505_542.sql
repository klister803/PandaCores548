DROP TABLE IF EXISTS `character_archaeology`;
CREATE TABLE `character_archaeology` (
  `guid` int(11) NOT NULL,
  `sites` text NOT NULL,
  `counts` text NOT NULL,
  `projects` text NOT NULL,
  PRIMARY KEY (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='Archaeology System';

DROP TABLE IF EXISTS `character_archaeology_finds`;
CREATE TABLE `character_archaeology_finds` (
  `guid` int(11) NOT NULL,
  `id` int(11) NOT NULL,
  `count` int(11) NOT NULL,
  `date` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`guid`,`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

INSERT INTO character_archaeology_finds (guid, id, count, date) 
SELECT guid, projectId, count, FROM_UNIXTIME(timeCreated) FROM character_archprojecthistory;

INSERT INTO character_archaeology (guid, sites, counts, projects)
SELECT guid, '', '', GROUP_CONCAT(projectId separator ' ') FROM character_archproject GROUP BY guid;

DROP TABLE IF EXISTS character_archproject;
DROP TABLE IF EXISTS character_archprojecthistory;

ALTER TABLE character_queststatus ADD COLUMN `mobcount5` smallint(5) unsigned NOT NULL DEFAULT '0' AFTER `mobcount4`;
ALTER TABLE character_queststatus ADD COLUMN `mobcount6` smallint(5) unsigned NOT NULL DEFAULT '0' AFTER `mobcount5`;
ALTER TABLE character_queststatus ADD COLUMN `mobcount7` smallint(5) unsigned NOT NULL DEFAULT '0' AFTER `mobcount6`;
ALTER TABLE character_queststatus ADD COLUMN `mobcount8` smallint(5) unsigned NOT NULL DEFAULT '0' AFTER `mobcount7`;
ALTER TABLE character_queststatus ADD COLUMN `mobcount9` smallint(5) unsigned NOT NULL DEFAULT '0' AFTER `mobcount8`;
ALTER TABLE character_queststatus ADD COLUMN `mobcount10` smallint(5) unsigned NOT NULL DEFAULT '0' AFTER `mobcount9`;

ALTER TABLE character_queststatus ADD COLUMN `itemcount5` smallint(5) unsigned NOT NULL DEFAULT '0' AFTER `itemcount4`;
ALTER TABLE character_queststatus ADD COLUMN `itemcount6` smallint(5) unsigned NOT NULL DEFAULT '0' AFTER `itemcount5`;
ALTER TABLE character_queststatus ADD COLUMN `itemcount7` smallint(5) unsigned NOT NULL DEFAULT '0' AFTER `itemcount6`;
ALTER TABLE character_queststatus ADD COLUMN `itemcount8` smallint(5) unsigned NOT NULL DEFAULT '0' AFTER `itemcount7`;
ALTER TABLE character_queststatus ADD COLUMN `itemcount9` smallint(5) unsigned NOT NULL DEFAULT '0' AFTER `itemcount8`;
ALTER TABLE character_queststatus ADD COLUMN `itemcount10` smallint(5) unsigned NOT NULL DEFAULT '0' AFTER `itemcount9`;

ALTER TABLE `character_currency`
ADD `curentcap` int(11) UNSIGNED DEFAULT '0' NOT NULL AFTER flags;

DROP TABLE `character_rated_bg`;

DROP TABLE IF EXISTS `character_brackets_info`;
CREATE TABLE `character_brackets_info` (
  `guid` int(11) NOT NULL,
  `bracket` smallint(6) NOT NULL,
  `rating` mediumint(9) NOT NULL DEFAULT '0',
  `best` mediumint(9) NOT NULL DEFAULT '0',
  `bestWeek` smallint(9) NOT NULL DEFAULT '0',
  `mmr` mediumint(9) NOT NULL DEFAULT '0',
  `games` int(11) NOT NULL DEFAULT '0',
  `wins` int(11) NOT NULL DEFAULT '0',
  `weekGames` mediumint(9) NOT NULL DEFAULT '0',
  `weekWins` mediumint(9) NOT NULL DEFAULT '0',
  PRIMARY KEY (`guid`,`bracket`),
  KEY `BracketID` (`bracket`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

ALTER TABLE `item_instance`
ADD `upgradeId` MEDIUMINT(8) UNSIGNED DEFAULT '0' NOT NULL AFTER transmogrifyId;

DROP TABLE `arena_team`;
DROP TABLE `arena_team_member`;

