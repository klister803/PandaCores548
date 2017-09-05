-- ТОЖЕ НА ЛАЙВ НЕ ЛИТЬ
-- Ладно, это нужно
ALTER TABLE `item_instance`
ADD COLUMN `isdonateitem`  tinyint(1) UNSIGNED NOT NULL DEFAULT 0 AFTER `text`;