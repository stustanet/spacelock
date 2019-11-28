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


-- secret signing key, shared with the door chip
do $$ begin
	create table signer(
		secret text not null
	);
	-- initialize the table with the firmware's default key
	insert into signer (secret)
	values ('AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=');
exception
	when duplicate_table then null;
end $$;


-- permission definitions for door users
create table if not exists permissions(
	id bigserial primary key not null,
	key text unique not null,                      -- user's uuid for logins
	reqid text unique not null,                    -- user'q account creation request id
	name text unique,                              -- some name
	granted_by bigint references permissions(id),  -- who enabled the user initially
	valid_from timestamp with time zone,
	valid_to timestamp with time zone,
	token_validity_time int not null default 0,    -- duration for token validity
	active boolean not null default false,         -- is the user enabled
	usermod boolean not null default false,        -- may this user modify other users
	keyupdate boolean not null default false,      -- may this user update the door key
	hidden boolean not null default false          -- hide the user from the user list
);


-- token and access grant log
create table if not exists log(
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
	payload_b64 text
) returns text as $$
	import base64
	import hmac
	import struct

	message_blob = struct.pack(
		'<qqB',
		int(now_timestamp) - validity_window_size,
		int(now_timestamp) + validity_window_size,
		message_type
	) + base64.b64decode(payload_b64.encode())
	signing_key_blob = base64.b64decode(signing_key.encode())

	signature = hmac.new(signing_key_blob, msg=message_blob, digestmod='sha256').digest()
	combined_blob = signature[:16] + message_blob
	return base64.b64encode(combined_blob).decode()
$$ language plpython3u;


-- check message signature of a key-update message
-- if signature is invalid, return null
-- return the extracted payload as base64
create or replace function extract_keyupdate(
	signing_key text,
	signed_msg text,
	now_timestamp double precision
) returns text as $$
import base64
import hashlib
import hmac
import struct


signing_key_blob = base64.b64decode(signing_key.encode())

signed_msg_blob = base64.b64decode(signed_msg)
signature_blob = signed_msg_blob[:16]
message_blob = signed_msg_blob[16:]

validity_start, validity_end, msgtype = struct.unpack_from(
    '<qqB',
    message_blob
)
payload = message_blob[struct.calcsize("<qqB"):]
sig = hmac.new(signing_key_blob, msg=message_blob, digestmod='sha256').digest()[:16]

# signature and message type check
if msgtype != 2 or sig != signature_blob:
    return None

# time validity check
if now_timestamp <= validity_start or now_timestamp >= validity_end:
    return None

# implementation like in the firmware
hash = hashlib.sha256()
hash.update(signing_key_blob)
hash.update(payload)
new_key = hash.digest()

return base64.b64encode(new_key).decode()
$$ language plpython3u;


-- generate a new secret 32 byte key
create or replace function keygen()
returns text as $$
import base64
import os

# key is 32 bytes long
new_key = os.urandom(32)
return base64.b64encode(new_key).decode()
$$ language plpython3u;


do $$ begin
	create type access_class as enum ('token', 'usermod', 'keyupdate');
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
			(what = 'keyupdate' and keyupdate is true) or
			(what = 'token')
		);

	if entry is null then
		return null;
	else
		return entry.id;
	end if;
end;
$$ language plpgsql
security definer;


-- token generation, this is the entry point for untrusted users
create or replace function gen_message(
	permission_key text,
	msg_type access_class,
	payload text
)
returns text as $$
declare entry permissions%ROWTYPE;
declare entry_id bigint;
declare now_time timestamp with time zone;
declare signing_key text;
declare token text;
declare token_duration int;
begin
	select now() into now_time;

	select can_access(permission_key, msg_type) into entry_id;
	select * into entry from permissions where id = entry_id;

	if entry is null then
		return null;
	end if;

	select secret into signing_key from signer order by id desc limit 1;
	if signing_key is null then
		return null;
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
		(case
		 when msg_type = 'token' then 1        -- message type 1: open door
		 when msg_type = 'keyupdate' then 2    -- message type 2: secret key update
		 else -1
		 end),
		payload
	) into token;

	insert into log (who, what) values (
		entry.id,
		format('emit %s: %s', msg_type, token)
	);

	return token;
end;
$$ language plpgsql
security definer;


-- create a door access token
create or replace function gen_token(
	permission_key text
)
returns text as $$
declare entry_id bigint;
declare entry permissions%ROWTYPE;
begin
	select can_access(permission_key, 'token') into entry_id;
	select * into entry from permissions where id = entry_id;

	if entry is null then
		return null;
	end if;

	return gen_message(
		permission_key,
		'token',
		encode(convert_to(entry.reqid, 'UTF8'), 'base64')
	);
end;
$$ language plpgsql
security definer;


-- create a key update token
create or replace function gen_keyupdate(
	permission_key text
)
returns text as $$
declare new_key text;
begin
	select keygen() into new_key;

	return gen_message(
		permission_key,
		'keyupdate',
		new_key
	);
end;
$$ language plpgsql
security definer;


-- update the key that is used for signing messages
-- this function is called after the door key was updated
create or replace function update_signingkey(
	permission_key text,
	update_message text
)
returns text as $$
declare now_time timestamp with time zone;
declare entry_id bigint;
declare entry permissions%ROWTYPE;
declare current_key text;
declare new_key text;
begin
	select now() into now_time;

	select can_access(permission_key, 'keyupdate') into entry_id;
	select * into entry from permissions where id = entry_id;

	if entry is null then
		return null;
	end if;

	select secret into current_key from signer order by id desc limit 1;
	if current_key is null then
		return null;
	end if;

	select extract_keyupdate(current_key,
	                         update_message,
	                         extract(epoch from now_time)) into new_key;
	if new_key is null then
		return null;
	end if;

	-- write the new key to the database!
	update signer set secret = new_key where secret = current_key;

	insert into log (who, what) values (
		entry.id,
		'update signingkey'
	);

	return 'ok';
end;
$$ language plpgsql
security definer;


-- create a message that random request identifier
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

	insert into permissions (key, reqid)
	values (
		hashed_new_key,
		ret.reqid
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
	_name text default null,
	_valid_from timestamp with time zone default null,
	_valid_to timestamp with time zone default null,
	_token_validity_time int default null,
	enable_usermod boolean default null
) returns text as $$
declare entry_id bigint;
declare entry permissions%ROWTYPE;
declare prev_state permissions%ROWTYPE;
declare changes text array;
begin
	select can_access(admin_token, 'usermod') into entry_id;
	select * into entry from permissions where id = entry_id;

	if entry is null then
		return null;
	end if;

	changes := array[]::text[];

	select * into prev_state from permissions where reqid = target_reqid;

	if _name is not null and _name != prev_state.name then
		update permissions set name = _name where reqid = target_reqid;
		select array_append(changes, 'name') into changes;
	end if;

	if _valid_from is not null and (prev_state.valid_from is null or _valid_from != prev_state.valid_from) then
		update permissions set valid_from = _valid_from where reqid = target_reqid;
		select array_append(changes, 'valid_from') into changes;
	end if;

	if _valid_to is not null and (prev_state.valid_to is null or _valid_to != prev_state.valid_to) then
		update permissions set valid_to = _valid_to where reqid = target_reqid;
		select array_append(changes, 'valid_to') into changes;
	end if;

	if _token_validity_time is not null and _token_validity_time != prev_state.token_validity_time then
		update permissions set token_validity_time = _token_validity_time where reqid = target_reqid;
		select array_append(changes, 'token_validity_time') into changes;
	end if;

	if enable_usermod is not null and enable_usermod != prev_state.usermod then
		update permissions set usermod = enable_usermod where reqid = target_reqid;
		select array_append(changes, 'usermod') into changes;
	end if;

	if cardinality(changes) = 0 then
		return 'none';
	end if;

	insert into log (who, what) values (
		entry.id,
		format('modify user: %s: %s', target_reqid, array_to_string(changes, ', '))
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
		return null;
	end if;

	select active into previous_state from permissions where reqid = target_reqid;

	if previous_state = _active then
		return null;
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


-- this function is used to activate an admin user
-- call this manually in the DB only, not from the webapp etc.
-- it uses no authorization!
create or replace function manual_admin_enable(
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
		format('manual admin user enabled: %s', target_reqid)
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


-- hide/unhide a user's account
-- that way we never actually delete users
create or replace function user_set_visibility(
	admin_token text,
	target_reqid text,
	hide boolean
) returns text as $$
declare entry_id bigint;
declare entry permissions%ROWTYPE;
begin
	select can_access(admin_token, 'usermod') into entry_id;
	select * into entry from permissions where id = entry_id;

	if entry is null then
		return null;
	end if;

	update permissions set hidden = hide where reqid = target_reqid;

	insert into log (who, what) values (
		entry.id,
		format('%shide user: %s',
		       case when hide then 'un' else '' end,
		       target_reqid)
	);

	return 'ok';
end;
$$ language plpgsql
security definer;


-- list all users
create or replace function user_list(
	permission_key text,
	show_hidden boolean default false
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
		from permissions
		where (permissions.hidden = false) or show_hidden;
end;
$$ language plpgsql
security definer;

-- aand we're done!
end;
