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
	'TESTC',
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
	'2024-03-16 18:46:11.349508+01'::timestamp with time zone,
	true,
	'ein wirklich schöner schlüssel für system A',
	'1',
	'\x7ea0ecec75b4da207aefdb170bd42a944488c8a7a0b07ebbd7c684dc0911a4ec0172ba3468777bf5ff59ac064e61a244ed7aeceb57ad6535f0b95cf99e598232'::bytea,
	'\x0172ba3468777bf5ff59ac064e61a244ed7aeceb57ad6535f0b95cf99e598232'::bytea
);


insert into signing_keys (system_id, key_id, version, is_active, comments, key_type, secret_key, verify_key)
values (
	'B',
	'B',
	'2024-03-31 20:00:12.869867+02'::timestamp with time zone,
	true,
	'ein wirklich schöner schlüssel für system B',
	'1',
	'\x318ec1020e839a98e0cac48b693e770c554cd71965476988a355cdc91c9010814390f10f9cd72828ce9ed30c974b24f9ba796f0909d8dee2f1824e10abe2f1f2'::bytea,
	'\x4390f10f9cd72828ce9ed30c974b24f9ba796f0909d8dee2f1824e10abe2f1f2'::bytea
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
	'A'
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
	'A'
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
	'A'
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
	'A'
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
	'A'
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
	'A'
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
	'A'
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
	'A'
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
	'A'
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
	'B'
);
insert into doors_x_systems (door_id, system_door_id, system_id)
values (
	101,
	'a',
	'A'
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
	'B'
);
insert into doors_x_systems (door_id, system_door_id, system_id)
values (
	102,
	'b',
	'A'
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
	'B'
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
	'B'
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
insert into doors_X_keyrings (system_door_id, system_id, keyring_id)
values (
	'1',
	'A',
	1
);
insert into doors_X_keyrings (system_door_id, system_id, keyring_id)
values (
	'2',
	'A',
	1
);
insert into doors_X_keyrings (system_door_id, system_id, keyring_id)
values (
	'3',
	'A',
	1
);
insert into doors_X_keyrings (system_door_id, system_id, keyring_id)
values (
	'a',
	'A',
	1
);

insert into keyrings (keyring_id, keyring_name, is_predefined, system_id, comments)
values (
	2,
	'gang 2',
	false,
	'A',
	'türen 1,9'
);
insert into doors_X_keyrings (system_door_id, system_id, keyring_id)
values (
	'1',
	'A',
	2
);
insert into doors_X_keyrings (system_door_id, system_id, keyring_id)
values (
	'9',
	'A',
	2
);


--
-- usr
--

insert into usr (usr_id,key, ref, name, system_id, granted_by, valid_from, valid_to, token_validity_time, active, hidden, comments)
values (
	1,
	pw_crypt('A', '46675c70-886d-409f-adb3-92e7eb2cd9b0'),
	'superadmin_A',
	'Superadmin A',
	'A',
	null,
	'2024-03-01 18:46:11'::timestamp with time zone,
	'infinity',
	3600,
	true,
	true,
	'Superadmin A, ausgang alle rechte in system A'
);

insert into usr (usr_id, key, ref, name, system_id, granted_by, valid_from, valid_to, token_validity_time, active, hidden, comments)
values (
	2,
	pw_crypt('A', 'b7ef10d5-1d98-4952-aac0-d19e49f23d97'),
	'superadmin_B',
	'Superadmin B',
	'B',
	null,
	'2024-03-01 18:46:11'::timestamp with time zone,
	'infinity',
	3600,
	true,
	true,
	'Superadmin A, ausgang alle rechte in system A'
);

insert into usr (usr_id, key, ref, name, system_id, granted_by, valid_from, valid_to, token_validity_time, active, hidden, comments)
values (
	3,
	pw_crypt('C', '4bff6567-a1e7-4de5-8a3f-d42de9689a69'),
	'superadmin_C',
	'Superadmin C',
	'C',
	null,
	'2024-03-01 18:46:11'::timestamp with time zone,
	'infinity',
	3600,
	true,
	true,
	'Superadmin C, ausgang alle rechte in system C'
);

insert into usr (usr_id, key, ref, name, system_id, granted_by, valid_from, valid_to, token_validity_time, active, hidden, comments)
values (
	101,
	pw_crypt('A', '88902ef8-0330-4f74-8ae9-d1a94391a697'),
	'manager_A1',
	'Manager A1',
	'A',
	1,
	'2024-03-01 18:46:11'::timestamp with time zone,
	'infinity',
	3600,
	true,
	false,
	'Manager A1, alle rechte in system A'
);

insert into usr (usr_id, key, ref, name, system_id, granted_by, valid_from, valid_to, token_validity_time, active, hidden, comments)
values (
	102,
	pw_crypt('B', '7b8e050b-e3b1-4a76-b1c8-b9bc672db3a6'),
	'manager_B1',
	'Manager B1',
	'B',
	2,
	'2024-03-01 18:46:11'::timestamp with time zone,
	'infinity',
	3600,
	true,
	false,
	'Manager B1, alle rechte in system B'
);

insert into usr (usr_id, key, ref, name, system_id, granted_by, valid_from, valid_to, token_validity_time, active, hidden, comments)
values (
	103,
	pw_crypt('C', '498561de-d44d-4e58-9ce8-dc2dabd9b5b9'),
	'manager_C1',
	'Manager C1',
	'C',
	3,
	'2024-03-01 18:46:11'::timestamp with time zone,
	'infinity',
	3600,
	true,
	false,
	'Manager B1, alle rechte in system B'
);

insert into usr (usr_id, key, ref, name, system_id, granted_by, valid_from, valid_to, token_validity_time, active, hidden, comments)
values (
	201,
	pw_crypt('A', '2409775b-47cf-47a5-8b47-cc480a039c51'),
	'usermanager_A1',
	'UserManager A1',
	'A',
	101,
	'2024-03-01 18:46:11'::timestamp with time zone,
	'infinity',
	3600,
	true,
	false,
	'Manager für User A1, managed User im system A'
);

insert into usr (usr_id, key, ref, name, system_id, granted_by, valid_from, valid_to, token_validity_time, active, hidden, comments)
values (
	202,
	pw_crypt('B', '25f6e891-f4ec-44f8-b6a3-562bb1305f5f'),
	'usermanager_B1',
	'UserManager B1',
	'B',
	102,
	'2024-03-01 18:46:11'::timestamp with time zone,
	'infinity',
	3600,
	true,
	false,
	'Manager für User B1, managed User im system B'
);

insert into usr (usr_id, key, ref, name, system_id, granted_by, valid_from, valid_to, token_validity_time, active, hidden, comments)
values (
	203,
	pw_crypt('C', '3007f47c-e2a3-4720-9c66-c5e8dcf6046a'),
	'usermanager_C1',
	'UserManager C1',
	'C',
	103,
	'2024-03-01 18:46:11'::timestamp with time zone,
	'infinity',
	3600,
	true,
	false,
	'Manager für User C1, managed User im system C'
);

insert into usr (usr_id, key, ref, name, system_id, granted_by, valid_from, valid_to, token_validity_time, active, hidden, comments)
values (
	1001,
	pw_crypt('A', '33f66113-18ef-4330-927b-b62a2eba1c19'),
	'user A01',
	'User A01',
	'A',
	201,
	'2024-03-01 18:46:11'::timestamp with time zone,
	'infinity',
	3600,
	true,
	false,
	'User A01'
);

insert into usr (usr_id, key, ref, name, system_id, granted_by, valid_from, valid_to, token_validity_time, active, hidden, comments)
values (
	1002,
	pw_crypt('A', 'de494af4-b2b5-459a-829b-b75fdf06db88'),
	'user A02',
	'User A02',
	'A',
	201,
	'2024-03-01 18:46:11'::timestamp with time zone,
	'infinity',
	3600,
	true,
	false,
	'User A02'
);

insert into usr (usr_id, key, ref, name, system_id, granted_by, valid_from, valid_to, token_validity_time, active, hidden, comments)
values (
	1003,
	pw_crypt('A', '539b8572-805d-4fd9-9f45-6c6aa40388f4'),
	'user A03',
	'User A03',
	'A',
	201,
	'2024-03-01 18:46:11'::timestamp with time zone,
	'infinity',
	3600,
	true,
	false,
	'User A03'
);

insert into usr (usr_id, key, ref, name, system_id, granted_by, valid_from, valid_to, token_validity_time, active, hidden, comments)
values (
	2001,
	pw_crypt('B', 'a045d8b3-2000-4684-a6b4-b624b39a72b0'),
	'user B01',
	'User B01',
	'B',
	202,
	'2024-03-01 18:46:11'::timestamp with time zone,
	'infinity',
	3600,
	true,
	false,
	'User B01'
);

insert into usr (usr_id, key, ref, name, system_id, granted_by, valid_from, valid_to, token_validity_time, active, hidden, comments)
values (
	3001,
	pw_crypt('C', 'b53212f5-05e8-4a82-8815-c2f0e133b325'),
	'user C01',
	'User C01',
	'C',
	203,
	'2024-03-01 18:46:11'::timestamp with time zone,
	'infinity',
	3600,
	true,
	false,
	'User C01'
);


--
-- permissions
--

insert into permissions (permission_id, usr_id, keyring_id, system_id, granted_by, valid_from, valid_to, active, comments)
values (
	1,
	1001,
	1,
	'A',
	101,
	'2024-03-01 18:46:11'::timestamp with time zone,
	'infinity',
	true,
	'keyring 1 für user A01 vom system A'
);
