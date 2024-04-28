create or replace trigger trigger_insert__systems__create_all_keyring
	after insert
	on systems
	for each row
	execute function trigger_insert__systems__create_all_keyring();

create or replace trigger trigger_insert__systems__create_default_roles_priv
	after insert
	on systems
	for each row
	execute function trigger_insert__systems__create_default_roles_priv();

create or replace trigger trigger_insert__doors_X_systems__add_to_all_keyring
	after insert
	on doors_X_systems
	for each row
	execute function trigger_insert__doors_X_systems__add_to_all_keyring();



