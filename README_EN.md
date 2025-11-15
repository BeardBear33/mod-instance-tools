# mod-instance-tools

### 🇨🇿 [Czech version](README.md)

## Description (EN)
This module allows you to:
- Reset individual or all instances
- Set a price for instance resets
- Set a limit for resetting individual instances
- Set a date and time for resetting the limit
- Teleport to VoA even when you don’t own Wintergrasp
- Set a price for teleporting to VoA

### Requirements  
Before using, make sure the database user from `WorldDatabaseInfo` (default `acore`) also has privileges for the new `customs` database:  

```sql
GRANT ALL PRIVILEGES ON customs.* TO 'acore'@'localhost';
FLUSH PRIVILEGES;
```

### ⚠️ Warning
The module uses its own **entry** in the tables `creature_template`, `creature_template_model`.
It is necessary to ensure that these IDs are not already occupied by other content in `acore_world`:

- **Custom creatures (`creature_template.entry`):**
  - `987456`, `987457`

If you already have other records in the database using these IDs, you must shift the numbers in the module and in the SQL to another free range.

**Notes:**
- The module uses its own `customs` DB where the tables set the price for instance reset and perform checks for how many times a player has reset and whether they have the selected limit until reset.


### Commands
.instance limit / l
.i limit / l
➝ Displays whether the player has already used a limit for something.


## Database tables and their descriptions
- instance_reset_limit = This table only writes and reads values when someone resets an instance, for correctly determining the reset limit. Do not touch this unless you want to remove someone's reached limit before the configured reset time.

- instance_reset_catalog
  - id = catalog number only
  - map_id = ID of the instance
  - difficulty = determines whether it is 10/25, normal/heroic ---- Not finished yet, until then: 0 raids are reset regardless of 10/25/normal/heroic difficulty and are treated as one, 1 only heroic dungeons are reset
  - is_raid = sets the category where it should appear, dungeon or raid
  - price_gold = gold price for resetting the instance
  - emblem_item = any item required to reset the instance
  - emblem_count = number of items required to reset the instance
  - enabled = enables or disables resetting of the given instance
  - comment = informational only


## License
This module is licensed under the [GNU General Public License v3.0 (GPL-3.0)](LICENSE).

