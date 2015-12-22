DELETE FROM spell_pet_auras WHERE petEntry IN (54569) and spellId IN (124416);
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES ('119053', 'spell_monk_transcendence_clone_visual');

insert into `spell_talent_linked_spell` (`spellid`, `spelllink`, `type`, `target`, `caster`, `comment`) values
('125660','108561','0','0','0','‘имвол дзуки'),
('-125660','-108561','0','0','0','‘имвол дзуки');
