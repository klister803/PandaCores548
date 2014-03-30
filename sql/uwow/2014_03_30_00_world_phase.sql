DROP TABLE phase_template;

INSERT INTO `phase_definitions` (`zoneId`, `entry`, `phasemask`, `phaseId`, `terrainswapmap`, `flags`, `comment`) VALUES
(4755, 1, 1, 0, 638, 0, 'Worgen Start location');

DELETE FROM `conditions` WHERE SourceGroup in (4756, 4755);
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `NegativeCondition`, `ErrorTextId`, `ScriptName`, `Comment`) VALUES
(23, 4755, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, '', 'shadow-phase for start worgen location');