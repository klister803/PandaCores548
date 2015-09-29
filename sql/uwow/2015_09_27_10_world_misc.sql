UPDATE `creature_template` SET `spell2`='0' WHERE (`entry`='5925');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES ('8178', 'spell_sha_grounding_totem');
UPDATE `spell_proc_event` SET `procFlags`='139264' WHERE (`entry`='8178') AND (`effectmask`='7');

DELETE FROM `spell_proc_event` WHERE (`entry`='974') AND (`effectmask`='7');
INSERT INTO `spell_proc_event` (`entry`, `procFlags`) VALUES ('974', '1048576');