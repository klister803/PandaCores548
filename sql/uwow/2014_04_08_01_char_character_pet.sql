update `character_pet` set `slot` = `slot` - 25 WHERE `slot` > 24 AND `slot` < 36;
update `character_pet` set `slot` = `slot` - 36 WHERE `slot` > 35 AND `slot` < 41;
update `character_pet` set `slot` = 25 WHERE `slot` = 100;
