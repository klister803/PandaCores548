CREATE TABLE `character_gameobject` (
  `guid` int(11) unsigned NOT NULL,
  `entry` int(11) NOT NULL DEFAULT '0',
  `respawnTime` int(11) NOT NULL DEFAULT '0',
  PRIMARY KEY (`guid`,`entry`),
  KEY `entry` (`entry`),
  KEY `respawnTime` (`respawnTime`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8
