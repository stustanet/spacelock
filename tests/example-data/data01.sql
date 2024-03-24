--
-- systems
--

insert into systems (system_id, system_name, is_foreign, comments)
values (
	'A',
	'TESTA',
	false,
	'test A'
);

insert into systems (system_id, system_name, is_foreign, comments)
values (
	'B',
	'TESTB',
	false,
	'test B'
);

insert into systems (system_id, system_name, is_foreign, comments)
values (
	'C',
	'TESTB',
	false,
	'test C, has no doors'
);

insert into systems (system_id, system_name, is_foreign, comments)
values (
	'X',
	'TESTX',
	true,
	'test X'
);


--
-- keys
--

insert into signing_keys (system_id, key_id, version, is_active, comments, key_type, secret_key, verify_key)
values (
	'A',
	'A',
	'2024-03-16 18:46:11.349508+01'::timestamp with zone,
	true,
	'ein wirklich schöner schlüssel für system A',
	'\x7ea0ecec75b4da207aefdb170bd42a944488c8a7a0b07ebbd7c684dc0911a4ec0172ba3468777bf5ff59ac064e61a244ed7aeceb57ad6535f0b95cf99e598232'::bytea,
	'\x0172ba3468777bf5ff59ac064e61a244ed7aeceb57ad6535f0b95cf99e598232'::bytea,
);


--
-- doors
--

insert into doors (door_id, door_name, owner_system_id, comments)
values (
	1,
	'testtür',
	'A',
	'diese tuer ist die schönste aller türen'
);
insert into doors_x_systems (door_id, system_door_id, system_id)
values (
	1,
	'1',
	'A',
);

insert into doors (door_id, door_name, owner_system_id, comments)
values (
	2,
	'testtür A2',
	'A',
	'diese tuer ist irgendeine blöde tür und gehört A'
);
insert into doors_x_systems (door_id, system_door_id, system_id)
values (
	2,
	'2',
	'A',
);

insert into doors (door_id, door_name, owner_system_id, comments)
values (
	3,
	'testtür A3',
	'A',
	'diese tuer ist irgendeine sehr schlaue tür und gehört A'
);
insert into doors_x_systems (door_id, system_door_id, system_id)
values (
	3,
	'3',
	'A',
);

insert into doors (door_id, door_name, owner_system_id, comments)
values (
	4,
	'testtür A4',
	'A',
	'diese tuer ist halt ne tür und gehört A'
);
insert into doors_x_systems (door_id, system_door_id, system_id)
values (
	4,
	'4',
	'A',
);

insert into doors (door_id, door_name, owner_system_id, comments)
values (
	5,
	'testtür A5',
	'A',
	'diese tuer ist halt ne tür und gehört A'
);
insert into doors_x_systems (door_id, system_door_id, system_id)
values (
	5,
	'5',
	'A',
);

insert into doors (door_id, door_name, owner_system_id, comments)
values (
	6,
	'testtür A6',
	'A',
	'diese tuer ist halt ne tür und gehört A'
);
insert into doors_x_systems (door_id, system_door_id, system_id)
values (
	6,
	'6',
	'A',
);

insert into doors (door_id, door_name, owner_system_id, comments)
values (
	7,
	'testtür A7',
	'A',
	'diese tuer ist halt ne tür und gehört A'
);
insert into doors_x_systems (door_id, system_door_id, system_id)
values (
	7,
	'7',
	'A',
);

insert into doors (door_id, door_name, owner_system_id, comments)
values (
	8,
	'testtür A8',
	'A',
	'diese tuer ist halt ne tür und gehört A'
);
insert into doors_x_systems (door_id, system_door_id, system_id)
values (
	8,
	'8',
	'A',
);

insert into doors (door_id, door_name, owner_system_id, comments)
values (
	9,
	'testtür A9',
	'A',
	'diese tuer ist halt ne tür und gehört A'
);
insert into doors_x_systems (door_id, system_door_id, system_id)
values (
	9,
	'9',
	'A',
);

insert into doors (door_id, door_name, owner_system_id, comments)
values (
	101,
	'testtür B1',
	'B',
	'diese tuer ist die hässlichste aller türen und gehört B, A kann sie auch nutzen'
);
insert into doors_x_systems (door_id, system_door_id, system_id)
values (
	101,
	'1',
	'B',
);
insert into doors_x_systems (door_id, system_door_id, system_id)
values (
	101,
	'a',
	'A',
);

insert into doors (door_id, door_name, owner_system_id, comments)
values (
	102,
	'testtür B2',
	'B',
	'diese tür ist auch nicht schön und gehärt B, A kann sie auch nutzen'
);
insert into doors_x_systems (door_id, system_door_id, system_id)
values (
	102,
	'2',
	'B',
);
insert into doors_x_systems (door_id, system_door_id, system_id)
values (
	102,
	'b',
	'A',
);

insert into doors (door_id, door_name, owner_system_id, comments)
values (
	103,
	'testtür B3',
	'B',
	'diese tür ist ne ganz akzeptable hübsche tür und gehört B'
);
insert into doors_x_systems (door_id, system_door_id, system_id)
values (
	103,
	'3',
	'B',
);

insert into doors (door_id, door_name, owner_system_id, comments)
values (
	104,
	'testtür B4',
	'B',
	'diese tür ist ne ganz akzeptable hübsche tür und gehört B'
);
insert into doors_x_systems (door_id, system_door_id, system_id)
values (
	104,
	'4',
	'B',
);


--
-- keyrings
--
insert into keyrings (keyring_id, keyring_name, is_predefined, system_id, comments)
values (
	1,
	'gang 1',
	false,
	'A',
	'türen 1,2,3,101'
);

insert into keyrings (keyring_id, keyring_name, is_predefined, system_id, comments)
values (
	2,
	'gang 2',
	false,
	'A',
	'türen 1,9'
);


--
-- usr
--

insert into usr (usr_id,key, reqid, name, system_id, granted_by, valid_from, valid_to, token_validity_time, active, usermod, manager, hidden, comments)
values (
	1,
	'superadmin_A',
	'7ffbd060-e9f6-11ee-87b0-28d244ae06bb',
	'Superadmin A',
	'A',
	null,
	'2024-03-01 18:46:11'::timestamp with zone,
	null,
	3600,
	true,
	true,
	true,
	true,
	'Superadmin A, ausgang alle rechte in system A'
);

insert into usr (usr_id, key, reqid, name, system_id, granted_by, valid_from, valid_to, token_validity_time, active, usermod, manager, hidden, comments)
values (
	2,
	'superadmin_B',
	'0496d3be-e9f8-11ee-a6d0-28d244ae06bb',
	'Superadmin B',
	'B',
	null,
	'2024-03-01 18:46:11'::timestamp with zone,
	null,
	3600,
	true,
	true,
	true,
	true,
	'Superadmin A, ausgang alle rechte in system A'
);

insert into usr (usr_id, key, reqid, name, system_id, granted_by, valid_from, valid_to, token_validity_time, active, usermod, manager, hidden, comments)
values (
	3,
	'manager_A1',
	'7ffbd060-e9f6-11ee-87b0-28d244ae06bb',
	'Superadmin A',
	'A',
	null,
	'2024-03-01 18:46:11'::timestamp with zone,
	1,
	3600,
	true,
	true,
	true,
	true,
	'Superadmin A, ausgang alle rechte in system A'
);

