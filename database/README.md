# Access Database

* Grants door access.
* The only trusted software component.

Needs PostgreSQL.

## setup order

- python dependencies: python3-libnacl
- as postgres
    - create role
    - create database owner role
    - load `setup_1010_base.sql` to activate extensions `plpython3u` and `pgcrypto`
    - load as user postgres `setup_0150_priv_code.sql` (because has extension language functions that need superuser privileges)
- as role user
    - load `setup_1020_code_table.sql` (functions that are used in tables)
    - load `setup_1030_tables.sql` (table structure)
    - load `setup_1040_views.sql` (table views)
    - load `setup_1050_code.sql` (main code for all spacelock features)
    - maybe load `setup_1051_code.sql` (compatibility layer for old website)
    - load `setup_1060_trigger.sql` (table trigger function hooks)

- load `truncate_tables.sql` to delete all table contents for easy testing

- `drop_*.sql` have to be loaded in inverse order of `setup_*` steps when modifications up until a step are needed.
    - for example: if you want to modify `setup_1030_tables`, you must drop trigger, code, views, tables (in that order).
      afterwards, they can be applied again.
    - if you add tables, you have also add an drop line to the drop script, if you add a trigger, also adapt `drop_trigger.sql`, etc etc

