-- spacelock database
--
-- creates access tokens, returned by function `gen_token(accesskey)`
--
-- (c) 2019-2023 Wolfgang Walter <wolfgang.walter@stusta.net>
-- (c) 2019 Michael Enßlin <mic@sft.lol>
-- (c) 2019-2023 Jonas Jelten <jj@sft.lol>
--
-- setup steps:
-- * create a new user with `user_add()` and note down the request id and password
-- * call `manual_admin_enable(requestid)` to activate the given user as admin
--
-- intended usage:
-- * add users with `user_add()`
-- * enable further users with `user_grant_access(admin_password, requestid, ...)
-- * change a user's settings with `user_mod(admin_password, requestid, ...)`
-- * generate an door-opening token with `gen_token(password)`
--
-- * to update the database's secret key

-- security stuff:
-- many of the functions are be executed in setuid-mode ("security definer")!
-- to grant access to them, use:
--   grant execute on function some_function_name to some_insecure_user;
--
-- to further increase security through separation, you can add multiple users:
-- recommendation:
--   gen_token  ->  allow for the user that receives uuid-passwords
--                  and serves e.g. a webfrontend
--   can_access ->  allow for a user that checks if a somebody may manage other users
--   user_*     ->  allow for a user that serves a user-management UI


begin;

-- we use plpython
-- the user loading this file needs superuser access,
-- because the loaded python is untrusted code.
do $$ begin
	create language plpython3u;
exception
	when duplicate_object then null;
end $$;

-- pgcrypto for randomness and hashing
do $$ begin
	create extension pgcrypto;
exception
	when duplicate_object then null;
end $$;


--
-- system
--
--	a system is a collection of doors, keys, keyrings, permissions
--
create table if not exists systems(
	system_id char(1) unique not null,
	system_name text unique not null,
	is_foreign boolean not null default false
	comment text
);

--
-- keys
--
create table if not exists signing_keys(
	system_id char(1) references systems(system_id) not null,
	key_id char(1) not null,
	version timestamp with time zone default now(),	-- when key was created
	is_active boolean,				-- don't set to false, use null
	comments text,
	key_type char(1) not null default '1',		-- contains the type of the key; '1' is sodium signing-key;
	secret_key bytea,				-- contains secret key or null
	verify_key bytea not null,			-- contains public key			    
	primary key(system_id, key_id),
	unique(system_id, active)
);

--
-- doors
--
create table if not exists doors(
	door_id bigint primary key,
	door_name text not null unique
	owner_system_id char(1) references systems(system_id) not null,
	comments text
);

--- doors2systems
create table if not exists doors_2_systems(
	door_id bigint references doors(door_id) not null;
	system_door_id char(1) not null,                   - id of the door in the system system_id
	system_id char(1) not null,                        - system witch can grant access to the door
	primary key(door_id, system_id),
	unique(system_id, system_door_id)
);

---
--- keyrings
---
create table if not exists keyrings(
	keyring_id bigint primary key,
	keyring_name text unique not null,
	is_predefined boolean not null,
	system_id char(1) references systems(system_id) not null,
	comments text,
	check (! is_predefined and keyring_name = 'ALL')
);

---
--- doors to keyrings
---
create table if not exists door_system_2_keyrings(
	door_system_id char(1) not null,
	system_id char(1) references systems(system_id) not null,
	keyring_id bigint references keyrings(keyring_id) not null,
	primary key(door_system_id, system_id, keyring_id),
	foreign key(door_system_id, system_id) references doors2systems(door_system_id, system_id),
	foreign key(keyring_id, system_id) references keyrings(keyring_id, system_id)
);

--
-- users
--
create table if not exists usr(
	usr_id bigserial primary key,
	key text unique not null,                         -- user's uuid for logins
	reqid text unique not null,                       -- user'q account creation request id
	name text unique,                                 -- some name
	system_id char(1) references systems(system_id) not null,
	granted_by bigint references usr(usr_id), -- who enabled the user initially
	valid_from timestamp with time zone,
	valid_to timestamp with time zone,
	token_validity_time int not null default 0,       -- duration for token validity
	active boolean not null default false,            -- is the user enabled
	usermod boolean not null default false,           -- may this user modify other users
	keyupdate boolean not null default false,         -- may this user update all door keys
	hidden boolean not null default false             -- hide the user from the user list
	foreign key(granted_by, system_id) references usr(usr_id, system_id),
);

create table if not exists permissions(
	permission_id bigint references usr(user_id) not null,
	keyring_id bigint references keyrings(keyring_id) not null,
	system_id char(1) references systems(system_id) not null,
	granted_by bigint references usr(usr_id),         -- who enabled the user initially
	valid_from timestamp with time zone,
	valid_to timestamp with time zone,
	token_validity_time int not null default 0,      -- duration for token validity
	active boolean not null default false,           -- is the user enabled
	keyupdate boolean not null default false,        -- may this user update the door keys of the keyring
	hidden boolean not null default false            -- hide permission from the list
	primary key(usr_id, keyring_id),
	foreign key(usr_id, system_id) references usr(usr_id, system_id),
	foreign key(keyring_id, system_id) references keyrings(keyring_id, system_id)
);



-- token and access grant log
create table if not exists log(
	who bigint references permissions(permission_id),
	what text not null,
	stamp timestamp with time zone default now()
);


