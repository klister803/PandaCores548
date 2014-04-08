-- Warlock
update `character_pet` set `slot` = `slot` - 25 WHERE `slot` > 24 AND `slot` < 36;
-- Stamped
update `character_pet` set `slot` = `slot` - 36 WHERE `slot` > 35 AND `slot` < 41;
-- Other
update `character_pet` set `slot` = 56 WHERE `slot` = 100;
