-- spacelock database functions
--
-- (c) 2019-2024 Wolfgang Walter <wolfgang.walter@stusta.net>
-- (c) 2019 Michael Enßlin <mic@sft.lol>
-- (c) 2019-2023 Jonas Jelten <jj@sft.lol>
--

--
-- given the secret return the verify-key
--
create or replace function verify_key(
	signing_type char(1),
	signing_secret bytea
) returns bytea as $$
	import nacl.signing

	if signing_key_type != '1':
		return None

	verify_key_encoded = nacl.signing.SigningKey.generate(signing_key).verify_key.encode()
	return verify_key_encoded
$$ language plpython3u
   set search_path = "$user", public;

--
-- given a secret create a signing key and return secret and verify-key
-- if the secret is an empty bytestring or is None than create a radnom secret
--
create or replace function create_signing_key_pair(
	p_type char(1),
	p_secret bytea,
	OUT o_secret bytea,
	OUT o_verify_key_encoded bytea
) as $$
	import nacl.signing
	import nacl.utils

	o_secret = None
	o_verify_key_encoded = None

	if p_key_type != '1':
		return

	if p_secret is None or p_secret == b'':
		p_secret = nacl.utils.random()
	
	l_secret_key = nacl.signing.SigningKey.generate(p_secret)
	o_verify_key_encoded = l_secret_key.verify_key.encode()
	o_secret = p_secret
$$ language plpython3u
   set search_path = "$user", public;

--
-- add a key to the table of signing keys
--
create or replace function add_signing_key(
	p_system_id char(1),
	p_key_id char(1),
	p_type char(1),
	p_secret bytea
) returns boolean as $$
declare
	l_secret bytea,
	l_verify_key_encoded bytea
begin
	select create_signing_key_pair(p_type, p_secret) into (l_secret, l_verify_key_enoced);

	if l_signing_secret is null then
		return false;
	end if

	insert into signing_keys as system_id, key_id, type, secret_key, verify_key
	values ( system_id, key_id, p_type, l_secret, l_verify_key_enoded );
	return FOUND;
$$ language plpgsql
   set search_path = "$user", public;


-- message signing in python, because we can
create or replace function sign_message(
	p_system_id char(1),
	p_key_id char(1),
	p_key_type char(1),
	p_secret bytea,
	p_payload bytea
	p_now_timestamp double precision,
	p_validity_window_size_sec int,
) returns bytea as $$
	import struct
	import nacl.signing

	if p_key_type != '1':
		return None

	message = struct.pack(
		'<IH',
		int((p_now_timestamp - p_validity_window_size_sec) / 60),
		int(p_validity_window_size_sec/60),
	) + p_system_id.encode() + p_key_id.encode() + p_payload
	secret_key = nacl.signing.SigningKey(p_secret)
	signed_message = secret_key.sign(message)
	return signed_message
$$ language plpython3u
   set search_path = "$user", public;


create or replace get_signing_key(
	p_system_id char(1)
) returns RECORD as $$
declare
	ret RECORD;
begin
	select key_type, key_id, secret_key into ret
	from keys
	where system_id = p_system_id and active = true;
	return ret
end;
$$ language plpgsql
   set search_path = "$user", public;
	

create or replace function create_message(
	p_system_id char(1),
	p_message_type char(1),
	p_payload_type char(1),
	p_payload bytea,
	p_now_timestamp timestamp with timestamp,
	p_validity_window_size_sec int,
) returns text as $$
declare
	l_key_id char(1);
	l_key_type char(1);
	l_secret_key bytea;
	l_signed_message bytea;
begin
	select get_signing_key(p_system_id) into l_key_type, l_key_id, l_secret;
	if not found then
		return null;
	end if;
	select sign_message(
		p_system_id,
		l_key_id,
		l_key_type,
		l_secret,
		convert_to(p_payload_type, 'UTF8') + p_payload,
		p_now_timestamp,
		p_validity_window_size_sec,
	) into l_signed_message;
	return l_signed_message
end;
$$ language plpgsql
   set search_path = "$user", public;

--
-- create a message to initialize a door
--
-- an update message has the following fields
--	message-type	I
--	system-id	uft8 encoded char
--	key-id		utf8 enocded char
--	key-type        utf8 encoded char
--	verify-key	byte-string (length depends on key-type)
--	door		uft8 encoded char
--
-- the door then belongs to system <system>
-- 
create or replace function create_message_init_door(
	p_owner_system_id char(1),		-- system the door itself think it is belonging too
	p_door_id char(1),			-- the id of the door
	p_system_id char(1),			-- the system the door should belong too
	now_timestamp timestamp with time zone,
	validity_window_size_sec int
) returns bytea as $$
declare
	l_key_id char(1);
	l_key_type char(1);
	l_verify_key bytea;
	is_foreign boolean;
begin
	-- the door already has to belong to the new system
	perform true from doors where door_id = p_door_id and owner_system_id = p_system_id;
	if not found then
		return null;
	end if;

	select key_id, key_type, verify_key
	into l_key_id, l_key_type, l_verify_key
	from signing_keys
	where system_id = p_system_id and is_active = true;
	if l_key_id is null then
		return null;
	end if;

	payload =    convert_to(p_system_id || l_key_id || l_key_type, 'UTF8')
	          || l_verify_key || convert_to(p_door_id, 'UTF8');

	return create_message(
		p_owner_system_id,
		now_timestamp,
		validity_window_size_sec,
		'I',
		payload
	)
end;
$$ language plpgsql
   set search_path = "$user", public;

--
-- create a message for all known doors of a system A which are doors of a system B
--	to add the newest key (highest version)
--
-- an update message has the following fields
--	message-type	U
--	system-id	uft8 encoded char
--	key-id		utf8 encoded char
--	key-type        utf8 encoded char
--	verify-key      byte-string (length depends from public-key-type)
--	doors           uft8-encodeder string
--				system_door_id,owner_door_id,...
--	\0
--	
create or replace function create_message_add_key(
	p_doors_owner_system_id char(1),
	p_system_id char(1),
	now_timestamp timestamp with timestamp,
	validity_window_size_sec int
) returns bytea as $$
declare
	l_new_key_id char(1);
	l_new_key_type char(1);
	l_new_verify_key bytea;
	l_new_key_version bigint;
	l_door_string text;
begin
	select key_id, key_type, verify_key, version
	into l_new_key_id, l_new_key_type, l_new_verify_key, l_new_key_version
	from signing_keys
	where system_id = p_system_id
	order by version desc
	limit 1;

	if not found then
		return null;
	end if;

	select STRING_AGG(
		d2sS.system_door_id || d2sO.system_door_id
		order d2sS.system_door_id, d2sO.system_door_id
	) as doors
	into l_door_string
	from	     doors as d
		join doors_2_systems as d2sO on d.door_id = d2sO.door_id
	        join doors_2_systems as d2sS on d2s0.door_id = d2sS.door_id
	where	d.owner_system_id = p_doors_owner_system_id
		and d2sS.system_id = p_system_id;

	if not found then
		return null;
	end if;

	payload =    convert_to(p_system_id || l_new_key_id || l_new_key_type, 'UTF8')
	          || l_new_verify_key || convert_to(l_door_string, 'UTF8') || '\x00'::bytea

	return create_message(
		p_doors_owner_system_id,
		now_timestamp,
		validity_window_size_sec,
		'U',
		payload
	)
end;
$$ language plpgsql
   set search_path = "$user", public;


--
-- create a message for all known doors of a system A which are doors of a system B
--	to flush all keys but the newest
--
-- an update message has the following fields
--	message-type	F
--	system-id	uft8 encoded char
--	key-id		utf8 encoded char
--	doors           uft8-encodeder string
--	\0
--	
create or replace function create_message_flush_keys(
	p_doors_owner_system_id char(1),
	p_system_id char(1),
	now_timestamp timestamp with timestamp,
	validity_window_size_sec int
) returns bytea as $$
declare
	l_active_key_id char(1);
	l_active_key_type char(1);
	l_door_string text;
begin
	select key_id, key_type
	into l_active_key_id, l_active_key_type
	from signing_keys
	where system_id = p_system_id and is_active = true;

	if not found then
		return null;
	end if;

	select STRING_AGG(system_door_id order by system_door_id)
	into l_door_string
	from	     doors
		join doors_2_systems on doors.door_id = doors_2_systems.door_id
	where     doors.owner_system_id = p_doors_owner_system_id
	      and doors_2_systems.system_id = p_system_id;

	if not found then
		return null;
	end if;

	payload =    convert_to(p_system_id || l_active_key_id, 'UTF8')
	          || convert_to(l_door_string, 'UTF8') || '\x00'::bytea

	return create_message(
		p_doors_owner_system_id,
		now_timestamp,
		validity_window_size_sec,
		'F',
		payload
	)
end;
$$ language plpgsql
   set search_path = "$user", public;


--
-- create a message for a user which opens all doors he is allowed to open
--
-- a message has the following fields
--	message-type	O
--	doors           uft8-encodeder string
--	\0
--	
create or replace function create_message_flush_keys(
	p_system_id char(1),
	p_usr bigint,
	p_now_timestamp timestamp with timestamp,
) returns bytea as $$
declare
	l_door_string text;
	l_valid_to timestamp with time zone;
	l_token_validity_time int;
	l_user_token_validity_time int;
	l_user_valid_from timestamp with time zone;
	l_user_valid_to timestamp with time zone;
begin
	select valid_from, valid_to, token_validity_time
	into l_user_valid_from, l_user_valid_to, l_user_token_validity_time
	where	    system_id = p_system_id and usr_id = p_usr
		and active = true
		and valid_from <= p_now_timestamp and p_now_timestamp < valid_to
	;

	if not found then
		return null;
	end if;

	select STRING_AGG(d2s.system_door_id order by d2s.system_door_id), min(p.valid_to), min(p.token_validity_time)
	into l_door_string, l_valid_to, l_token_validity_time
	from	     doors as d
		join doors_2_systems as d2s on d.door_id = d2s.door_id
		join doors_X_keyrings as dXk
			on dXk.system_door_id = d2s.system_door_id and sXk.system_id = d2s.system_id
		join keyrings as k on k.system_id = dXk.system_id and k.keyring_id = d2k.keyring_id
		join permissions as p on p.system_id = k.system_id and p.keyring_id = k.keyring_id
		join usr as u on u.system_id = p.system_id and u.usr_id = p.usr_id
	where	    p.system_id = p_system_id and p.usr_id = p_usr
		and u.active = true and p.active = true
		and u.valid_from <= p_now_timestamp and p_now_timestamp < u.valid_to
		and (p.valid_from <= p_now_timestamp or p.valid_from is null)
		and (p_now_timestamp < p.valid_to or p.valid_from is null)
	;

	if not found then
		return null;
	end if;

	if l_token_validity_time > l_user_token_validity_time then
		l_token_validity_time = l_user_token_validity_time
	end if;
	if l_valid_to > l_user_valid_to then
		l_valid_to = l_user_valid_to
	end if;
	if l_valid_to - p_now_timestamp < l_token_validity_time then
		l_token_validity_time = l_valid_to - p_now_timestamp
	end if;

	payload =  convert_to(l_door_string, 'UTF8') || '\x00'::bytea

	return create_message(
		p_doors_owner_system_id,
		p_now_timestamp,
		l_token_validity_time,
		'O',
		payload
	)
end;
$$ language plpgsql
   set search_path = "$user", public;






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
returns json as $$
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

	select secret into signing_key from signer limit 1;
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

	return json_build_object(
		'token', token,
		'valid_until', now_time + make_interval(secs => token_duration)
	);
end;
$$ language plpgsql
security definer;


-- create a door access token
create or replace function gen_token(
	permission_key text
)
returns json as $$
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
returns json as $$
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

	select secret into current_key from signer limit 1;
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

	if _name is not null and (prev_state.name is null or _name != prev_state.name) then
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

-- get valid_to of a user
create or replace function get_valid_to(
    permission_key text
) returns timestamp with time zone as $$
declare entry_id bigint;
declare entry permissions%ROWTYPE;
begin
	select can_access(permission_key, 'token') into entry_id;
	select * into entry from permissions where id = entry_id;

	if entry is null then
		return null;
	end if;

	return entry.valid_to;
end;
$$ language plpgsql
security definer;

-- aand we're done!
end;
