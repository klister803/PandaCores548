REPLACE INTO `areatrigger_scripts` (`entry`, `ScriptName`) VALUES ('9267', 'at_siege_of_orgrimmar_portal_to_orgrimmar');

-- teleport from org.
REPLACE INTO `spell_target_position` (`id`, `target_map`, `target_position_x`, `target_position_y`, `target_position_z`, `target_orientation`) VALUES
('149407', '1136', '748', '1113', '357', '0'),
('148034', '1136', '1322.31', '-5285.94', '9', '0'); -- alliance 

-- ServerToClient: SMSG_UPDATE_OBJECT (0x1792) Length: 55681 ConnectionIndex: 0 Time: 10/08/2014 19:36:05.000 Type: Unknown Opcode Type Number: 952691

-- alliance 223459
