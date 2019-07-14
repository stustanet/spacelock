-- spacelock database
--
-- creates access tokens, returned by function `gen_token(accesskey)`
--
-- (c) 2019 Wolfgang Walter <wolfgang.walter@stusta.net>
-- (c) 2019 Michael Enßlin <mic@sft.mx>
-- (c) 2019 Jonas Jelten <jj@sft.mx>
--
-- setup steps:
-- * create a new user with `user_add()` and note down the request id and password
-- * call `first_admin_enable(requestid)` to activate the given user as admin
-- * enable further users with `user_grant_access(admin_password, requestid, ...)
-- * change a user's validity time with `user_mod(admin_password, ...)`
--
-- regular usage:
-- * generate an door-opening token with `gen_token(password)`

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


-- secret signing key, shared with the door chip
create table if not exists signer (
	id bigserial not null,
	secret text not null
);


-- permission definitions for door users
create table if not exists permissions (
	id bigserial primary key not null,
	key text unique not null,
	reqid text unique not null,
	name text unique,
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
		key = crypt(permission_key, '$2a$06$lolspacelock1337salt42') and
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
create or replace function gen_token (
	permission_key text
)
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

	select greatest(
		least(entry.token_validity_time,
		      extract(epoch from (entry.valid_to - now()))),
		0)
	into token_duration;

	select sign_message(
		signing_key,
		extract(epoch from now_time),
		token_duration,
		1,    -- message type 1: open door
		entry.reqid
	) into token;

	insert into log (who, what) values (
		entry.id,
		format('create access token: %s', token)
	);

	return token;
end;
$$ language plpgsql
security definer;


-- create a random request identifier
create or replace function gen_reqid()
returns text as $$
	import random
	parts = ["%04d" % random.randint(0,9999) for _ in range(4)]
	return '-'.join(parts)
$$ language plpython3u;


do $$ begin
	create type new_user_info as (reqid text, key text);
exception
	when duplicate_object then null;
end $$;


-- add a user by name, this can be called by anyone
create or replace function user_add()
returns new_user_info as $$

declare ret new_user_info;
declare hashed_new_key text;
declare new_id bigint;
begin
	select gen_reqid() into ret.reqid;
	select gen_random_uuid()::text into ret.key;

	-- create blowfish hash
	select crypt(ret.key, '$2a$06$lolspacelock1337salt42')
	into hashed_new_key;

	insert into permissions (key, reqid,
	                         token_validity_time,
	                         active, usermod) values (
		hashed_new_key,
		ret.reqid,
		0,
		false,
		false
	) returning id into new_id;

	insert into log (who, what) values (
		new_id,
		format('add user: %s', ret.reqid)
	);

	return ret;
end;
$$ language plpgsql
security definer;


-- change a user's validity times
create or replace function user_mod(
	admin_token text,
	target_reqid text,
	set_name text,
	_valid_from timestamp with time zone,
	_valid_to timestamp with time zone,
	_token_validity_time int,
	enable_usermod boolean default false
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
		name = set_name,
		valid_from = _valid_from,
		valid_to = _valid_to,
		token_validity_time = _token_validity_time,
		usermod = enable_usermod
	where
		reqid = target_reqid;

	insert into log (who, what) values (
		entry.id,
		format('modify user: %s', target_reqid)
	);

	return 'ok';
end;
$$ language plpgsql
security definer;


-- set the user active flag
create or replace function user_set_active(
	admin_token text,
	target_reqid text,
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

	select active into previous_state from permissions where reqid = target_reqid;

	if previous_state = _active then
		return NULL;
	end if;

	update permissions
	set
		active = _active
	where
		reqid = target_reqid;

	insert into log (who, what) values (
		entry.id,
		format('%s user: %s',
		       case when _active then 'enable' else 'disable' end,
		       target_reqid)
	);

	return 'ok';
end;
$$ language plpgsql
security definer;


-- grant access to a newly created user
create or replace function user_grant_access(
	admin_token text,
	target_reqid text,
	name text,
	valid_from timestamp with time zone default now(),
	valid_to timestamp with time zone default (now() + interval '31' day),
	token_validity_time int default 86400  -- 24 hours
) returns text as $$
declare entry_id bigint;
begin
	select can_access(admin_token, 'usermod') into entry_id;

	if entry_id is null then
		return null;
	end if;

	if user_mod(admin_token, target_reqid, name,
	            valid_from, valid_to, token_validity_time) is null then
		return null;
	end if;

	if user_enable(admin_token, target_reqid) is null then
		return null;
	end if;

	update permissions set
		granted_by = entry_id
	where
		reqid = target_reqid;

	return 'ok';
end;
$$ language plpgsql
security definer;


-- this function is used to activate the first admin user
-- it uses no authorization!
create or replace function first_admin_enable(
	target_reqid text,
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
		reqid = target_reqid;

	insert into log (what) values (
		format('first admin user enabled: %s', target_reqid)
	);

	return 'ok';
end;
$$ language plpgsql;


-- disable an existing user
create or replace function user_disable(
	admin_token text,
	target_reqid text
) returns text as $$
begin
	return user_set_active(admin_token, target_reqid, false);
end;
$$ language plpgsql
security definer;


-- enable an existing user
create or replace function user_enable(
	admin_token text,
	target_reqid text
) returns text as $$
begin
	return user_set_active(admin_token, target_reqid, true);
end;
$$ language plpgsql
security definer;


-- remove a user's account
create or replace function user_del(
	admin_token text,
	target_reqid text
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
	where reqid = target_reqid;

	insert into log (who, what) values (
		entry.id,
		format('delete user: %s', target_reqid)
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
	reqid text,
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
	select * into entry from permissions where permissions.id = entry_id;

	if entry is null then
		return;
	end if;

	return query
		select
			permissions.id,
			permissions.reqid,
			permissions.name,
			permissions.granted_by,
			permissions.valid_from,
			permissions.valid_to,
			permissions.token_validity_time,
			permissions.active,
			permissions.usermod
		from permissions;
end;
$$ language plpgsql
security definer;

-- aand we're done!
