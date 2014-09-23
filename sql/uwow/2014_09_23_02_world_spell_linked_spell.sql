delete from spell_linked_spell where spell_effect in (146120, 146128, -146128, -146120);
INSERT INTO `spell_linked_spell` (`spell_trigger`, `spell_effect`, `type`, `caster`, `target`, `hastalent`, `hastalent2`, `chance`, `cooldown`, `type2`, `hitmask`, `learnspell`, `comment`) VALUES 
(-23920, -146120, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 'Remove visual effect'),
(-871, -146128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 'Remove visual effect'),
(871, 146128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 'Shield Wall (warrior)(visual with shield)'),
(23920, 146120, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 'Spell Reflection (warrior)(visual with shield)');
