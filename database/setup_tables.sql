-- spacelock database
--
-- (c) 2019-2023 Wolfgang Walter <wolfgang.walter@stusta.net>
-- (c) 2019 Michael Enßlin <mic@sft.lol>
-- (c) 2019-2023 Jonas Jelten <jj@sft.lol>
--

--
-- system
--
--	a system is a collection of doors, keys, keyrings, permissions
--
create table if not exists systems (
	system_id char(1) unique not null,
	system_name text unique not null,
	is_foreign boolean not null default false,
	comments text
);

--
-- keys
--
create table if not exists signing_keys (
	system_id char(1) not null references systems(system_id) on delete cascade,
	key_id char(1) not null,
	version timestamp with time zone default now(),	-- when key was created
	is_active boolean,				-- don't set to false, use null
	comments text not null default '',
	key_type char(1) not null default '1',		-- contains the type of the key; '1' is sodium signing-key;
	secret_key bytea,				-- contains secret key / secret or null
	verify_key bytea not null,			-- contains public key			    
	primary key(system_id, key_id),
	unique(system_id, is_active)
);

--
-- doors
--
create table if not exists doors (
	door_id bigint primary key,
	door_name text not null,
	owner_system_id char(1) not null references systems(system_id) on delete cascade,
	unique (owner_system_id, door_name),
	comments text not null default ''
);

--- doors_X_systems
create table if not exists doors_X_systems (
	door_id bigint not null references doors(door_id) on delete cascade,
	system_door_id char(1) not null,				-- id of the door in the system system_id
	system_id char(1) not null
		references systems(system_id) on delete cascade,	-- system witch can grant access to the door
	primary key(door_id, system_id),
	unique(system_id, system_door_id)
);

---
--- keyrings
---
create table if not exists keyrings (
	keyring_id bigint primary key,
	keyring_name text not null,
	is_predefined boolean not null,
	system_id char(1) not null references systems(system_id) on delete cascade,
	comments text not null default '',
	unique (system_id, keyring_name),
	unique (system_id, keyring_id),
	check (not is_predefined or keyring_name = 'ALL')
);

---
--- doors to keyrings
---
create table if not exists doors_X_keyrings (
	system_door_id char(1) not null,				-- door_id in this system
	system_id char(1) not null
		references systems(system_id) on delete cascade,	-- system_id the keyring belongs to
	keyring_id bigint not null
		references keyrings(keyring_id) on delete cascade,	-- the keyring id
	primary key (system_id, system_door_id, keyring_id),
	foreign key (system_id, system_door_id) references doors_X_systems(system_id, system_door_id)
		on delete cascade,
	foreign key (system_id, keyring_id) references keyrings(system_id, keyring_id)
		on delete cascade
);

--
-- users
--
create table if not exists usr (
	usr_id bigint primary key default gen_random_bigint(),		-- user's id
	key uuid unique not null default gen_random_uuid(),		-- user's id / pw for logins
	reqid uuid unique not null default gen_random_uuid(),		-- user's account creation request id
	name text,							-- some name
	ref text,							-- a reference to foreign database systems
	system_id char(1) not null
		references systems(system_id) on delete cascade,
	granted_by bigint references usr(usr_id) on delete set null,	-- who enabled the user initially
	valid_from timestamp with time zone not null default now(),	-- account cannot be used before
	valid_to timestamp with time zone not null default 'infinity',	-- account cannot be used after
	token_validity_time int not null default 0,			-- duration for token validity
	active boolean not null default false,				-- is the user enabled
	hidden boolean not null default false,				-- hide the user from the user list
	comments text not null default '',
	foreign key (granted_by, system_id) references usr(usr_id, system_id) on delete set null,
	unique (system_id, usr_id),
	unique (system_id, name),
	unique (system_id, ref),
	check (token_validity_time >= 0 and token_validity_time < 32767*60)
);

--
-- permissions
--
create table if not exists permissions (
	permission_id bigint primary key default gen_random_bigint(),
	key uuid unique not null default gen_random_uuid(),		-- user's permission uuid for logins
	usr_id bigint not null references usr(usr_id) on delete cascade,
	keyring_id bigint not null references keyrings(keyring_id) on delete cascade,
	system_id char(1) not null references systems(system_id) on delete cascade,
	granted_by bigint references usr(usr_id) on delete set null,	-- who enabled the door for this user
	valid_from timestamp with time zone not null default now(),	-- a user may not open the doors before
	valid_to timestamp with time zone not null default 'infinity',	-- a user may not open the doors after
	active boolean not null default false,				-- is this permission enabled
	comments text not null default '',
	unique (usr_id, keyring_id),
	foreign key (system_id, usr_id) references usr(system_id, usr_id),
	foreign key (system_id, keyring_id) references keyrings(system_id, keyring_id)
);

--
-- privileges
--
create table if not exists privs (
	priv_id bigint primary key,
	priv_name text unique not null,
	comments text not null default ''
);
insert into privs (
	priv_id, priv_name, comments
) values (
	  1, 'can_add_user', 'Can add a user'
), (
	  2, 'can_mod_user', 'Can change an existing user'
), (
	  3, 'can_del_user', 'Can remove a user'
), (
	  4, 'can_prolonge_user', 'Can prolong a user'
), (
	100, 'can_add_mod_del_permissions', 'Can add/modify/delete permissions to a user'
), (
	101, 'can_add_mod_del_door', 'Can add/modify/delete a door'
), (
	201, 'can_add_mod_del_keyring', 'Can add/modify/delete a keyring'
), (
	301, 'can_add_mod_del_signing_key', 'Can add/modify/delete a signing key'
), (
	401, 'can_create_I_message', 'Can create a initialisation message for a door'
), (
	402, 'can_create_U_message', 'Can create a U message to programm the newest signing key'
), (
	403, 'can_create_F_message', 'Can create a F message to delete all non active keys'
) on conflict do nothing;

--
-- roles
--
create table if not exists roles (
	role_id bigint not null,
	system_id char(1) not null references systems(system_id) on delete cascade,
	role_name text not null,
	comment text default '',
	primary key(role_id, system_id),
	unique (role_name, system_id)
);

--
-- roles_X_priv
--
create table if not exists roles_X_privs (
	priv_id bigint not null references privs(priv_id) on delete cascade,
	role_id bigint not null,
	system_id char(1) not null references systems(system_id) on delete cascade,
	primary key(priv_id, role_id, system_id),
	foreign key (system_id, role_id) references roles(system_id, role_id) on delete cascade
);

--
-- usr_X_roles
--
create table if not exists usr_X_roles (
	usr_id bigint not null references usr(usr_id) on delete cascade,
	role_id bigint not null,
	system_id char(1) not null references systems(system_id) on delete cascade,
	granted_by bigint references usr(usr_id) on delete set null,
	valid_from timestamp with time zone not null default now(),
	valid_to timestamp with time zone not null default 'infinity',
	comments text not null default '',
	primary key(usr_id, role_id, system_id),
	foreign key (system_id, usr_id) references usr(system_id, usr_id),
	foreign key (system_id, role_id) references roles(system_id, role_id) on delete cascade
);

--
-- token and access grant log
--
create table if not exists log (
	who bigint references permissions(permission_id) on delete set null,
	what text not null default '',
	stamp timestamp with time zone default now()
);

