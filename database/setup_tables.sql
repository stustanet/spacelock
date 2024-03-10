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
	system_id char(1) references systems(system_id) not null,
	key_id char(1) not null,
	version timestamp with time zone default now(),	-- when key was created
	is_active boolean,				-- don't set to false, use null
	comments text,
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
	owner_system_id char(1) references systems(system_id) not null,
	unique (owner_system_id, door_name),
	comments text
);

--- doors2systems
create table if not exists doors_2_systems (
	door_id bigint references doors(door_id) not null,
	system_door_id char(1) not null,                   -- id of the door in the system system_id
	system_id char(1) not null,                        -- system witch can grant access to the door
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
	system_id char(1) references systems(system_id) not null,
	comments text,
	unique (system_id, keyring_name),
	unique (system_id, keyring_id),
	check (not is_predefined and keyring_name = 'ALL')
);

---
--- doors to keyrings
---
create table if not exists doors_X_keyrings (
	system_door_id char(1) not null,					-- door_id in this system
	system_id char(1) references systems(system_id) not null,		-- system_id the keyring belongs to
	keyring_id bigint references keyrings(keyring_id) not null,		-- the keyring id
	primary key (system_id, system_door_id, keyring_id),
	foreign key (system_id, system_door_id) references doors_2_systems(system_id, system_door_id),
	foreign key (system_id, keyring_id) references keyrings(system_id, keyring_id)
);

--
-- users
--
create table if not exists usr (
	usr_id bigserial primary key,
	key text unique not null,				-- user's uuid for logins
	reqid text unique not null,				-- user's account creation request id
	name text,						-- some name
	system_id char(1) references systems(system_id) not null,
	granted_by bigint references usr(usr_id),		-- who enabled the user initially
	valid_from timestamp with time zone,
	valid_to timestamp with time zone,
	token_validity_time int not null default 0,		-- duration for token validity
	active boolean not null default false,			-- is the user enabled
	usermod boolean not null default false,			-- may this user modify other users
	keyupdate boolean not null default false,		-- may this user update all door keys
	hidden boolean not null default false			-- hide the user from the user list
	comments text,
	foreign key (granted_by, system_id) references usr(usr_id, system_id),
	unique (system_id, usr_id),
	unique (system_id, name)
);

--
-- permissions
--
create table if not exists permissions (
	permission_id bigserial primary key,
	keyring_id bigint references keyrings(keyring_id) not null,
	usr_id bigint references usr(usr_id) not null,
	system_id char(1) references systems(system_id) not null,
	granted_by bigint references usr(usr_id),		-- who enabled the user initially
	valid_from timestamp with time zone,
	valid_to timestamp with time zone,
	token_validity_time int not null default 0,		-- duration for token validity
	active boolean not null default false,			-- is the user enabled
	keyupdate boolean not null default false,		-- may this user update the door keys of the keyring
	hidden boolean not null default false,			-- hide permission from the list
	comments text,
	unique (usr_id, keyring_id),
	foreign key (system_id, usr_id) references usr(system_id, usr_id),
	foreign key (system_id, keyring_id) references keyrings(system_id, keyring_id)
);

--
-- token and access grant log
--
create table if not exists log (
	who bigint references permissions(permission_id),
	what text not null,
	stamp timestamp with time zone default now()
);

