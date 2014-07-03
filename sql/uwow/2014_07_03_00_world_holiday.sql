DELETE FROM game_event_gameobject WHERE guid in (SELECT guid from gameobject WHERE id = 217851);
DELETE FROM `gameobject` WHERE id = 217851;
INSERT INTO `gameobject` (`id`, `map`, `zoneId`, `areaId`, `spawnMask`, `phaseMask`, `position_x`, `position_y`, `position_z`, `orientation`, `rotation0`, `rotation1`, `rotation2`, `rotation3`, `spawntimesecs`, `animprogress`, `state`, `isActive`) VALUES
( 217851, 870, 5840, 5840, 1, 1, 1322.29, 460.914, 457.784, 3.87661, 0, 0, 0.933225, -0.359292, 300, 0, 1, 0);
INSERT INTO game_event_gameobject (eventEntry, guid) SELECT 1, guid FROM gameobject WHERE id = 187892;