--
-- compabitility code for old server
--
create or replace function compat_can_access(
	p_system_id char(1),
	p_usr_key text,
	p_what text
) returns bigint as $$
declare
	l_now_time timestamp with time zone;
	l_usr_id bigint;
	l_msg signed_message;
begin
	select now() into l_now_time;
	select usr_id into l_usr_id from usr_by_pw(p_system_id, p_usr_key);
	if l_usr_id is null then
		return null;
	end if;

	if p_what = 'token' then
		l_msg = create_message_open_doors(p_system_id, l_usr_id, l_now_time);
		if l_msg is null then
			return null;
		end if;
		return l_usr_id;
	end if;

	if usr_has_compat_priv(p_system_id, l_usr_id, p_what, l_now_time) then
		return l_usr_id;
	end if;

	return null;
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
	l_now_time timestamp with time zone;
	l_usr_id bigint;
	l_msg signed_message;
begin
	select now() into l_now_time;
	select usr_id into l_usr_id from usr_by_pw(p_system_id, p_usr_key);
	if l_usr_id is null then
		return null;
	end if;
	l_msg = create_message_open_doors(p_system_id, l_usr_id, l_now_time);
	if l_msg is null then
		return null;
	end if;
	return json_build_object(
		'token', encode(l_msg.signed_msg, 'base64'),
		'valid_until', l_msg.msg_timestamp + make_interval(secs => l_msg.validity_window_size_sec)
	);
end;
$$ language plpgsql
   set search_path = "$user", public;

--
-- 
--
