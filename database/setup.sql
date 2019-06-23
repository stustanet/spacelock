-- spacelock database
--
-- creates access tokens, returned by function `gen_token(accesskey)`
--
-- (c) 2019 Wolfgang Walter <wolfgang.walter@stusta.net>
-- (c) 2019 Michael Enßlin <mic@sft.mx>
-- (c) 2019 Jonas Jelten <jj@sft.mx>


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
	description text not null,
	key text unique not null,
	valid_from timestamp with time zone not null,
	valid_to timestamp with time zone not null,
	token_validity_time int not null,
	active boolean not null
);


-- token creation log
create table if not exists log (
	who bigint references permissions(id),
	token text not null,
	stamp timestamp with time zone default now()
);


-- message signing in python, because we can
create or replace function sign_message (
	signing_key text,
	now_timestamp double precision,
	validity_window_size int,
	message_type int,
	description text
) returns text as $$
	import base64
	import hmac
	import struct

	message_blob = struct.pack(
		'<qqB',
		int(now_timestamp) - validity_window_size,
		int(now_timestamp) + validity_window_size,
		message_type
	) + description.encode()
	signing_key_blob = base64.b64decode(signing_key.encode())

	signature = hmac.new(signing_key_blob, msg=message_blob, digestmod='sha256').digest()
	combined_blob = signature[:16] + message_blob
	return base64.b64encode(combined_blob).decode()
$$ language plpython3u;


-- token generation, this is the entry point for untrusted users
-- this function will be executed in setuid-mode!
-- to grant access, use:
--   create role spacelock;
--   grant execute on function gen_token to some_insecure_user;
create or replace function gen_token (permission_key text)
returns text as $$
declare entry permissions%ROWTYPE;
declare now_time timestamp with time zone;
declare signing_key text;
declare token text;
declare token_validity_time int;
begin
	select now() into now_time;

	select * into entry
	from permissions
	where
		key = permission_key and
		active is true and
		valid_from <= now_time and
		valid_to >= now_time;

	if entry is NULL then
		return NULL;
	end if;

	select secret into signing_key from signer order by id desc limit 1;
	if signing_key is NULL then
		return NULL;
	end if;

	select sign_message(
		signing_key,
		extract(epoch from now_time),
		entry.token_validity_time,
		0,    -- message type 0: open door
		entry.description
	) into token;

	insert into log (who, token, stamp) values (
		entry.id,
		token,
		now_time
	);

	return token;
end;
$$ language plpgsql
security definer;


-- aand we're done!
