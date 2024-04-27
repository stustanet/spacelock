--
-- compabitility code for old server
--

create or replace function compat_can_access(
	p_system_id char(1),
	p_usr_key text,
	what access_class
) returns bigint as $$
declare usr_entry permissions%ROWTYPE;
declare now_time timestamp with time zone;
begin
	select now() into now_time;

	select * into usr_entry
	from usr
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

--
-- create a door access token
--
create or replace function compat_gen_token(
	p_system_id char(1),
	p_usr_key text
) returns json as $$
declare
	l_usr_id bigint;
	msg signed_message;
begin
	select now() into now_time;
	select compat_can_open_doors(p_system_id, p_usr_key) into l_usr_id;
	if l_usr_id is null then
		return null;
	end if;
	msg = create_message_open_doors(p_system_id, l_usr_id);
	if msg is null then
		return null;
	end if;
	return json_build_object(
		'token', encode(msg.signed_msg, 'base64'),
		'valid_until', msg.msg_timestamp + make_interval(secs => msg.validity_window_size_sec)
	);
end;
$$ language plpgsql
   set search_path = "$user", public;

--
-- 
--
