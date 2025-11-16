# mod-instance-tools

### 🇬🇧 [English version](README_EN.md)

## Popis (CZ)  
Tento modul umožňuje:  
- Resetovat jednotlivé nebo všechny instance
- Nastavit cenu pro reset instancí
- Nastavit limit pro reset jednotlivých instancí
- Nastavit datum a čas pro resetování limitu
- Teleportovat do VoA, i když nevlastní Wintergrasp
- Nastavit cenu pro teleport do VoA

### Požadavky  
Před použitím je nutné zajistit, aby uživatel databáze z `WorldDatabaseInfo` (standardně `acore`) měl práva i na novou databázi `customs`:  

```sql
GRANT ALL PRIVILEGES ON customs.* TO 'acore'@'localhost';
FLUSH PRIVILEGES;
```

### ⚠️ Upozornění
Modul používá vlastní **entry** v tabulkách `creature_template`, `creature_template_model`.  
Je nutné zajistit, že tato ID nejsou v `acore_world` již obsazená jiným obsahem:

- **Vlastní creatures (`creature_template.entry`):**
  - `987456`, `987457`
  
Pokud máš v databázi již jiné záznamy s těmito ID, je potřeba čísla v modulu i v SQL posunout na jiný volný rozsah.

**Poznámky:**
- Modul používá vlastní DB `customs`, ve které se v tabulkách se nastavuje cena pro reset instance a provádí kontrola, kdo kolikrát resetoval a zda má vybraný limit do resetu.

### Příkazy
.instance limit / l
.i limit / l
➝ Vypíše, zda už má na něco použitý limit.

## Databázové tabulky a jejich popis
- instance_reset_limit = Tato tabulka pouze zapisuje a čte hodnoty, když někdo resetuje instanci, pro správné určení limitu na reset. Zde nesahat, pokud nechcete jenom někomu odebrat dosažený limit před nastaveným resetem.
- instance_reset_catalog
  - id = pouze katalogové číslo
  - map_id = ID pro instanci
  - difficulty = použitelné pouze pro raidy. 0 = 10man Normal, 1 = 25man Normal, 2 = 10man Heroic, 3 = 25man Heroic, 4 = všechny varianty. Pozor: pokud nastavíte hodnotu 4, nesmíte používat hodnoty 0, 1, 2, 3, a naopak.
  - price_gold = cena ve zlatě pro reset instance.
  - emblem_item = libovolný item potřebný pro reset instance.
  - emblem_count = počet itemů potřebných pro reset instance.
  - enabled = povolí nebo zakáže reset dané instance.
  - comment = pouze informativní.

## License
This module is licensed under the [GNU General Public License v3.0 (GPL-3.0)](LICENSE).


