-- World boss reset catalog for mod-instance-tools (customs DB)
CREATE TABLE IF NOT EXISTS `customs`.`worldboss_reset_catalog` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `creature_id` int unsigned NOT NULL COMMENT 'acore_world.creature.id1 (creature template entry)',
  `price_gold` int unsigned NOT NULL DEFAULT '0',
  `emblem_item` int unsigned NOT NULL DEFAULT '0',
  `emblem_count` int unsigned NOT NULL DEFAULT '0',
  `enabled` tinyint(1) NOT NULL DEFAULT '1',
  `display_name` varchar(255) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `comment` varchar(255) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  PRIMARY KEY (`id`),
  UNIQUE KEY `uq_creature_id` (`creature_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
  COMMENT='World boss early respawn catalog';

INSERT IGNORE INTO `customs`.`worldboss_reset_catalog`
  (`id`, `creature_id`, `price_gold`, `emblem_item`, `emblem_count`, `enabled`, `display_name`, `comment`)
VALUES
  (1, 18728, 50, 0, 0, 1, '', 'Doom Lord Kazzak (Hellfire Peninsula)'),
  (2, 17711, 50, 0, 0, 1, '', 'Doomwalker (Shadowmoon Valley)');
