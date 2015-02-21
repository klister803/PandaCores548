delete from creature where id = 73536;
INSERT INTO `creature` (`id`, `map`, `zoneId`, `areaId`, `spawnMask`, `phaseMask`, `modelid`, `equipment_id`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `spawndist`, `currentwaypoint`, `curhealth`, `curmana`, `MovementType`, `npcflag`, `npcflag2`, `unit_flags`, `dynamicflags`, `isActive`) VALUES 
(73536, 1136, 6738, 6738, 16632, 1, 0, 0, 1427.97, 356.15, 289.19, 4.91, 120, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
(73536, 1136, 6738, 6738, 16632, 1, 0, 0, 1454.52, 355.82, 289.19, 4.46, 120, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
(73536, 1136, 6738, 6738, 16632, 1, 0, 0, 783.54, 1168.18, 356.15, 4.22, 120, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
(73536, 1136, 6738, 6738, 16632, 1, 0, 0, 691.1, 1149.94, 356.15, 5.84, 120, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

update creature_template set IconName = 'interact', npcflag = 16777216 where entry = 73536;

delete from npc_spellclick_spells where npc_entry = 73536;
INSERT INTO `npc_spellclick_spells` (`npc_entry`, `spell_id`, `cast_flags`, `user_type`) VALUES 
(73536, 148034, 1, 0);

delete from spell_target_position where id = 148034;
INSERT INTO `spell_target_position` (`id`, `target_map`, `target_position_x`, `target_position_y`, `target_position_z`, `target_orientation`) VALUES 
(148034, 1136, 1439.95, -5013.35, 12.287, 1.647);