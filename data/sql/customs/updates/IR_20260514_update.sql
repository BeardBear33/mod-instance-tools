-- Optional custom label for reset NPC gossip / .instance limit (empty = DBC map name + difficulty suffix).
ALTER TABLE `customs`.`instance_reset_catalog`
  ADD COLUMN `display_name` varchar(255)
  COLLATE utf8mb4_unicode_ci
  NOT NULL DEFAULT ''
  AFTER `enabled`;
