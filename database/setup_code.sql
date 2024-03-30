-- spacelock database functions
--
-- (c) 2019-2024 Wolfgang Walter <wolfgang.walter@stusta.net>
-- (c) 2019 Michael Enßlin <mic@sft.lol>
-- (c) 2019-2023 Jonas Jelten <jj@sft.lol>
--


--
-- add a key to the table of signing keys
--
create or replace function add_signing_key (
	p_system_id char(1),
	p_key_id char(1),
	p_type char(1)
) returns boolean as $$
declare
	l_secret_key bytea;
	l_verify_key bytea;
begin
	select *
	into l_secret_key, l_verify_key
	from create_signing_key_pair(p_type);

	if l_secret_key is null then
		return false;
	end if;

	insert into signing_keys ( system_id, key_id, key_type, secret_key, verify_key )
	values ( p_system_id, p_key_id, p_type, l_secret_key, l_verify_key );
	return found;
end;
$$ language plpgsql
   set search_path = "$user", public;


--
-- get the actual active signing key
--
create or replace function get_signing_key (
	p_system_id char(1)
) returns setof active_signing_key_data as $$
begin
	return query
		select *
		from active_signing_key_data
		where system_id = p_system_id;
end;
$$ language plpgsql
   set search_path = "$user", public;
	

--
-- create and sign a message
--
create or replace function create_message (
	p_system_id char(1),
	p_payload_type char(1),
	p_payload bytea,
	p_now_timestamp timestamp with time zone,
	p_validity_window_size_sec int
) returns bytea as $$
declare
	l_key_id char(1);
	l_key_type char(1);
	l_secret_key bytea;
	l_signed_message bytea;
	l_now_utc_timestamp double precision;
begin
	select key_id, key_type, secret_key
	into l_key_id, l_key_type, l_secret_key
	from get_signing_key(p_system_id);
	if not found then
		return null;
	end if;
	select extract(epoch from (p_now_timestamp at time zone 'UTC'))
	into l_now_utc_timestamp;
	select *
	into l_signed_message
	from sign_message(
		p_system_id,
		l_key_id,
		l_key_type,
		l_secret_key,
		convert_to(p_payload_type, 'UTF8') || p_payload,
		l_now_utc_timestamp,
		p_validity_window_size_sec
	);
	return l_signed_message;
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
create or replace function create_message_init_door (
	p_owner_system_id char(1),		-- system the door itself think it is belonging too
	p_system_door_id char(1),		-- the id of the door
	now_timestamp timestamp with time zone,
	validity_window_size_sec int
) returns bytea as $$
declare
	l_key_id char(1);
	l_key_type char(1);
	l_verify_key bytea;
	is_foreign boolean;
	l_payload bytea;
begin
	-- the door already has to belong to the new system
	perform true from doors as d join doors_X_systems as dXs on d.door_id = dXs.door_id
	where dXs.system_id = p_owner_system_id and dXs.system_door_id = p_system_door_id;
	if not found then
		return null;
	end if;

	select key_id, key_type, verify_key
	into l_key_id, l_key_type, l_verify_key
	from signing_keys
	where system_id = p_owner_system_id and is_active = true;
	if not found then
		return null;
	end if;

	select    convert_to(p_owner_system_id || l_key_id || l_key_type, 'UTF8')
	       || l_verify_key || convert_to(p_system_door_id, 'UTF8')
	into l_payload;

	return create_message(
		p_owner_system_id,
		'I',
		l_payload,
		now_timestamp,
		validity_window_size_sec
	);
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
create or replace function create_message_add_key (
	p_doors_owner_system_id char(1),
	p_system_id char(1),
	now_timestamp timestamp with time zone,
	validity_window_size_sec int
) returns bytea as $$
declare
	l_new_key_id char(1);
	l_new_key_type char(1);
	l_new_verify_key bytea;
	l_new_key_version timestamp with time zone;
	l_door_string text;
	l_payload bytea;
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
		dXsS.system_door_id || dXsO.system_door_id, ''
		order by dXsS.system_door_id || dXsO.system_door_id
	) as doors
	into l_door_string
	from	     doors as d
		join doors_X_systems as dXsO on d.door_id = dXsO.door_id
	        join doors_X_systems as dXsS on dXsO.door_id = dXsS.door_id
	where	d.owner_system_id = p_doors_owner_system_id
		and dXsS.system_id = p_system_id;

	if l_door_string is null then
		l_door_string := '';
	end if;

	l_payload =    convert_to(p_system_id || l_new_key_id || l_new_key_type, 'UTF8')
	            || l_new_verify_key || convert_to(l_door_string, 'UTF8') || '\x00'::bytea;

	return create_message(
		p_doors_owner_system_id,
		'U',
		l_payload,
		now_timestamp,
		validity_window_size_sec
	);
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
create or replace function create_message_flush_keys (
	p_doors_owner_system_id char(1),
	p_system_id char(1),
	now_timestamp timestamp with time zone,
	validity_window_size_sec int
) returns bytea as $$
declare
	l_active_key_id char(1);
	l_active_key_type char(1);
	l_door_string text;
	l_payload bytea;
begin
	select key_id, key_type
	into l_active_key_id, l_active_key_type
	from signing_keys
	where system_id = p_system_id and is_active = true;

	if not found then
		return null;
	end if;

	select STRING_AGG(system_door_id, '' order by system_door_id)
	into l_door_string
	from	     doors
		join doors_X_systems on doors.door_id = doors_X_systems.door_id
	where     doors.owner_system_id = p_doors_owner_system_id
	      and doors_X_systems.system_id = p_system_id;

	if not found then
		return null;
	end if;

	l_payload =    convert_to(p_system_id || l_active_key_id, 'UTF8')
	            || convert_to(l_door_string, 'UTF8') || '\x00'::bytea;

	return create_message(
		p_doors_owner_system_id,
		'F',
		l_payload,
		now_timestamp,
		validity_window_size_sec
	);
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
create or replace function create_message_open_doors (
	p_system_id char(1),
	p_usr bigint,
	p_now_timestamp timestamp with time zone
) returns bytea as $$
declare
	l_door_string text;
	l_valid_to timestamp with time zone;
	l_user_token_validity_time int;
	l_user_token_validity_iv interval;
	l_token_validity_time int;
	l_user_valid_from timestamp with time zone;
	l_user_valid_to timestamp with time zone;
	l_payload bytea;
begin
	select valid_from, valid_to, token_validity_time
	into l_user_valid_from, l_user_valid_to, l_user_token_validity_time
	from usr
	where	    system_id = p_system_id and usr_id = p_usr
		and active = true
		and valid_from <= p_now_timestamp
		and p_now_timestamp <= valid_to
	;

	if not found then
		return null;
	end if;

	l_user_token_validity_iv = l_user_token_validity_time * '1 sec'::interval;

	select STRING_AGG(dXs.system_door_id, ''
		order by dXs.system_door_id), min(p.valid_to)
	into l_door_string, l_valid_to
	from	     doors as d
		join doors_X_systems as dXs on d.door_id = dXs.door_id
		join doors_X_keyrings as dXk
			on dXk.system_door_id = dXs.system_door_id and dXk.system_id = dXs.system_id
		join keyrings as k on k.system_id = dXk.system_id and k.keyring_id = dXk.keyring_id
		join permissions as p on p.system_id = k.system_id and p.keyring_id = k.keyring_id
		join usr as u on u.system_id = p.system_id and u.usr_id = p.usr_id
	where	    p.system_id = p_system_id and p.usr_id = p_usr
		and u.active = true and p.active = true
		and u.valid_from <= p_now_timestamp and p_now_timestamp <= u.valid_to
		and p.valid_from <= p_now_timestamp and p_now_timestamp <= p.valid_to
	;

	if l_door_string is null then
		l_door_string = '';
		l_valid_to = null;
	end if;

	if l_valid_to is null then
		l_valid_to = l_user_valid_to;
	elsif l_user_valid_to < l_valid_to then
		l_valid_to = l_user_valid_to;
	end if;

	if l_valid_to < p_now_timestamp + l_user_token_validity_iv then
		l_user_token_validity_iv = l_valid_to - p_now_timestamp;
	end if;
	l_token_validity_time = extract(epoch from l_user_token_validity_iv)::int;

	l_payload = convert_to(l_door_string, 'UTF8') || '\x00'::bytea;

	return create_message(
		p_system_id,
		'O',
		l_payload,
		p_now_timestamp,
		l_token_validity_time
	);
end;
$$ language plpgsql
   set search_path = "$user", public;

