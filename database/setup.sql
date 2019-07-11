-- spacelock database
--
-- creates access tokens, returned by function `gen_token(accesskey)`
--
-- (c) 2019 Wolfgang Walter <wolfgang.walter@stusta.net>
-- (c) 2019 Michael Enßlin <mic@sft.mx>
-- (c) 2019 Jonas Jelten <jj@sft.mx>
--
-- setup steps:
-- * create a new user with `user_add(username)` and note down the password
-- * call `first_admin_enable(username)` to activate the given user as admin
-- * enable further users with `user_grant_access(admin_password, target_username)


-- we use plpython
-- the user loading this file needs superuser access,
-- because the loaded python is untrusted code.
do $$ begin
	create language plpython3u;
exception
	when duplicate_object then null;
end $$;


-- secret signing key, shared with the door chip
create table if not exists signer (
	id bigserial not null,
	secret text not null
);


-- permission definitions for door users
create table if not exists permissions (
	id bigserial primary key not null,
	key text unique not null,
	name text unique not null,
	granted_by bigint references permissions(id),
	valid_from timestamp with time zone,
	valid_to timestamp with time zone,
	token_validity_time int not null,
	active boolean not null,
	usermod boolean not null
);


-- token and access grant log
create table if not exists log (
	who bigint references permissions(id),
	what text not null,
	stamp timestamp with time zone default now()
);


-- message signing in python, because we can
create or replace function sign_message(
	signing_key text,
	now_timestamp double precision,
	validity_window_size int,
	message_type int,
	name text
) returns text as $$
	import base64
	import hmac
	import struct

	message_blob = struct.pack(
		'<qqB',
		int(now_timestamp) - validity_window_size,
		int(now_timestamp) + validity_window_size,
		message_type
	) + name.encode()
	signing_key_blob = base64.b64decode(signing_key.encode())

	signature = hmac.new(signing_key_blob, msg=message_blob, digestmod='sha256').digest()
	combined_blob = signature[:16] + message_blob
	return base64.b64encode(combined_blob).decode()
$$ language plpython3u;


do $$ begin
	create type access_class as enum ('token', 'usermod');
exception
	when duplicate_object then null;
end $$;


-- permission verification is done here:
create or replace function can_access(permission_key text, what access_class)
returns bigint as $$
declare entry permissions%ROWTYPE;
declare now_time timestamp with time zone;
begin
	select now() into now_time;

	select * into entry
	from permissions
	where
		key = crypt(permission_key, key) and
		active is true and
		valid_from <= now_time and
		valid_to >= now_time and (
			(what = 'usermod' and usermod is true) or
			(what = 'token')
		);

	if entry is NULL then
		return null;
	else
		return entry.id;
	end if;
end;
$$ language plpgsql
security definer;


-- token generation, this is the entry point for untrusted users
-- this function will be executed in setuid-mode!
-- to grant access, use:
--   grant execute on function gen_token to some_insecure_user;
create or replace function gen_token(permission_key text)
returns text as $$
declare entry_id bigint;
declare entry permissions%ROWTYPE;
declare now_time timestamp with time zone;
declare signing_key text;
declare token text;
declare token_duration int;
begin
	select now() into now_time;

	select can_access(permission_key, 'token') into entry_id;
	select * into entry from permissions where id = entry_id;

	if entry is NULL then
		return NULL;
	end if;

	select secret into signing_key from signer order by id desc limit 1;
	if signing_key is NULL then
		return NULL;
	end if;

	select least(entry.token_validity_time,
	             extract(epoch from (entry.valid_to - now())))
	into token_duration;

	select sign_message(
		signing_key,
		extract(epoch from now_time),
		token_duration,
		0,    -- message type 0: open door
		entry.name
	) into token;

	insert into log (who, what) values (
		entry.id,
		format('create access token: %s', token)
	);

	return token;
end;
$$ language plpgsql
security definer;


-- add a user by name, this can be called by anyone
create or replace function user_add(
	name text
) returns text as $$
declare new_key text;
declare new_id bigint;
begin
	select gen_random_uuid()::text into new_key;

	insert into permissions (key, name,
	                         token_validity_time,
	                         active, usermod) values (
		crypt(new_key, gen_salt('bf')),
		name,
		0,
		false,
		false
	) returning id into new_id;

	insert into log (who, what) values (
		new_id,
		format('add user: %s', name)
	);

	return new_key;
end;
$$ language plpgsql
security definer;


-- change a user's validity times
create or replace function user_mod(
	admin_token text,
	target_name text,
	_valid_from timestamp with time zone,
	_valid_to timestamp with time zone,
	_token_validity_time int,
	_usermod boolean default false
) returns text as $$
declare entry_id bigint;
declare entry permissions%ROWTYPE;
begin
	select can_access(admin_token, 'usermod') into entry_id;
	select * into entry from permissions where id = entry_id;

	if entry is null then
		return NULL;
	end if;

	update permissions
	set
		valid_from = _valid_from,
		valid_to = _valid_to,
		token_validity_time = _token_validity_time,
		usermod = _usermod
	where
		name = target_name;

	insert into log (who, what) values (
		entry.id,
		format('modify user: %s', target_name)
	);

	return 'ok';
end;
$$ language plpgsql
security definer;


-- set the user active flag
create or replace function user_set_active(
	admin_token text,
	target_name text,
	_active boolean
) returns text as $$
declare entry_id bigint;
declare entry permissions%ROWTYPE;
declare previous_state bool;
begin
	select can_access(admin_token, 'usermod') into entry_id;
	select * into entry from permissions where id = entry_id;

	if entry is null then
		return NULL;
	end if;

	select active into previous_state from permissions where name = target_name;

	if previous_state = _active then
		return NULL;
	end if;

	update permissions
	set
		active = _active
	where
		name = target_name;

	insert into log (who, what) values (
		entry.id,
		format('%s user: %s',
		       case when _active then 'enable' else 'disable' end,
		       target_name)
	);

	return 'ok';
end;
$$ language plpgsql
security definer;


-- grant access to a newly created user
create or replace function user_grant_access(
	admin_token text,
	target_name text,
	valid_from timestamp with time zone default now(),
	valid_to timestamp with time zone default (now() + interval '31' day),
	token_validity_time int default 86400  -- 24 hours
) returns text as $$
begin
	if user_mod(admin_token, target_name, valid_from, valid_to, token_validity_time) is null then
		return null;
	end if;
	if user_enable(admin_token, target_name) is null then
		return null;
	end if;
	return 'ok';
end;
$$ language plpgsql
security definer;


-- this function is used to activate the first admin user
-- it uses no authorization!
create or replace function first_admin_enable(
	target_name text,
	_valid_from timestamp with time zone default now(),
	_valid_to timestamp with time zone default (now() + interval '31' day),
	_token_validity_time int default 86400  -- 24 hours
) returns text as $$
begin
	update permissions
	set
		valid_from = _valid_from,
		valid_to = _valid_to,
		token_validity_time = _token_validity_time,
		active = true,
		usermod = true
	where
		name = target_name;

	insert into log (what) values (
		format('first admin user enabled: %s', target_name)
	);

	return 'ok';
end;
$$ language plpgsql;


-- update a user password, requires the old password
create or replace function user_new_password(
	permission_key text
) returns text as $$
declare entry permissions%ROWTYPE;
declare new_key text;
begin
	select * into entry
	from permissions
	where
		key = crypt(permission_key, key);

	if entry is null then
		return null;
	end if;

	select gen_random_uuid()::text into new_key;

	update permissions
	set
		key = crypt(new_key, gen_salt('bf'))
	where
		id = entry.id;

	insert into log (who, what) values (
		entry.id,
		format('new password for user: %s', entry.name)
	);

	return new_key;
end;
$$ language plpgsql
security definer;


-- disable an existing user
create or replace function user_disable(
	admin_token text,
	target_name text
) returns text as $$
begin
	return user_set_active(admin_token, target_name, false);
end;
$$ language plpgsql
security definer;


-- enable an existing user
create or replace function user_enable(
	admin_token text,
	target_name text
) returns text as $$
begin
	return user_set_active(admin_token, target_name, true);
end;
$$ language plpgsql
security definer;


-- remove a user's account
create or replace function user_del(
	admin_token text,
	target_name text
) returns text as $$
declare entry_id bigint;
declare entry permissions%ROWTYPE;
begin
	select can_access(admin_token, 'usermod') into entry_id;
	select * into entry from permissions where id = entry_id;

	if entry is null then
		return NULL;
	end if;

	delete from permissions
	where name = target_name;

	insert into log (who, what) values (
		entry.id,
		format('delete user: %s', target_name)
	);

	return 'ok';
end;
$$ language plpgsql
security definer;


-- list all users
create or replace function user_list(
	permission_key text
)
returns table (
	id bigint,
	name text,
	granted_by bigint,
	valid_from timestamp with time zone,
	valid_to timestamp with time zone,
	token_validity_time int,
	active boolean,
	usermod boolean
) as $$
declare entry_id bigint;
declare entry permissions%ROWTYPE;
begin
	select can_access(permission_key, 'usermod') into entry_id;
	select * into entry from permissions where id = entry_id;

	if entry is null then
		return;
	end if;

	return query
		select
			(id, name, granted_by, valid_from,
			valid_to, token_validity_time, active,
			usermod)
		from permissions;
end;
$$ language plpgsql
security definer;

-- aand we're done!
