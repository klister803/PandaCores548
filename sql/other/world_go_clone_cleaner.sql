/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET NAMES utf8 */;
/*!40014 SET FOREIGN_KEY_CHECKS=0 */;


DROP TABLE IF EXISTS `gameobject_copy`;
CREATE TABLE `gameobject_copy` (
  `guid` int(10) unsigned NOT NULL AUTO_INCREMENT COMMENT 'Global Unique Identifier',
  `id` mediumint(8) unsigned NOT NULL DEFAULT '0' COMMENT 'Gameobject Identifier',
  `map` smallint(5) unsigned NOT NULL DEFAULT '0' COMMENT 'Map Identifier',
  `zoneId` int(8) unsigned NOT NULL DEFAULT '0',
  `areaId` int(8) unsigned NOT NULL DEFAULT '0',
  `spawnMask` int(11) unsigned NOT NULL DEFAULT '1',
  `phaseMask` int(11) unsigned NOT NULL DEFAULT '1',
  `position_x` float NOT NULL DEFAULT '0',
  `position_y` float NOT NULL DEFAULT '0',
  `position_z` float NOT NULL DEFAULT '0',
  `orientation` float NOT NULL DEFAULT '0',
  `rotation0` float NOT NULL DEFAULT '0',
  `rotation1` float NOT NULL DEFAULT '0',
  `rotation2` float NOT NULL DEFAULT '0',
  `rotation3` float NOT NULL DEFAULT '0',
  `spawntimesecs` int(11) NOT NULL DEFAULT '0',
  `animprogress` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `state` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `isActive` tinyint(1) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`guid`)
) ENGINE=MyISAM  DEFAULT CHARSET=utf8 ROW_FORMAT=FIXED COMMENT='Gameobject System' AUTO_INCREMENT=445360 ;
INSERT INTO gameobject_copy select * FROM gameobject;

-- Добавляем индексы для быстрой работы обработчика
ALTER TABLE `gameobject` ADD INDEX(`phaseMask`),
ADD INDEX(`spawnMask`),
ADD INDEX(`position_x`),
ADD INDEX(`position_y`),
ADD INDEX(`position_z`);

ALTER TABLE `gameobject_copy` ADD INDEX(`phaseMask`),
ADD INDEX(`spawnMask`),
ADD INDEX(`position_x`),
ADD INDEX(`position_y`),
ADD INDEX(`position_z`);

-- Удаление дубликатов
DELETE FROM gameobject WHERE guid in(SELECT gameobject.guid FROM `gameobject_copy` WHERE gameobject.id = gameobject_copy.id AND gameobject_copy.map = gameobject.map AND gameobject.guid != gameobject_copy.guid AND gameobject.position_x = gameobject_copy.position_x AND gameobject.position_y = gameobject_copy.position_y AND gameobject.position_z = gameobject_copy.position_z AND gameobject.spawnMask = gameobject_copy.spawnMask AND gameobject.phaseMask = gameobject_copy.phaseMask);

-- Чистка Фаз от дублей.
DELETE FROM gameobject WHERE guid in(SELECT gameobject.guid FROM `gameobject_copy` WHERE gameobject.id = gameobject_copy.id AND gameobject_copy.map = gameobject.map AND gameobject.guid != gameobject_copy.guid AND gameobject.position_x = gameobject_copy.position_x AND gameobject.position_y = gameobject_copy.position_y AND gameobject.position_z = gameobject_copy.position_z AND gameobject.spawnMask = gameobject_copy.spawnMask AND gameobject.phaseMask < gameobject_copy.phaseMask AND (gameobject_copy.phaseMask & gameobject.phaseMask) = gameobject.phaseMask);

-- Очистка аддонов и линков на гуид
ALTER TABLE `creature_addon` ADD INDEX(`path_id`);
DELETE FROM `creature_addon` WHERE guid not in (select guid from gameobject);
DELETE FROM waypoint_data WHERE id not in (SELECT path_id FROM creature_addon);
ALTER TABLE creature_addon DROP INDEX path_id;

-- Удаление временной таблицы
DROP TABLE gameobject_copy;

-- Удаление индексов
ALTER TABLE gameobject DROP INDEX position_x;
ALTER TABLE gameobject DROP INDEX position_y;
ALTER TABLE gameobject DROP INDEX position_z;
ALTER TABLE gameobject DROP INDEX phaseMask;
ALTER TABLE gameobject DROP INDEX spawnMask;