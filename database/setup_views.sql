-- spacelock database
--
-- (c) 2019-2023 Wolfgang Walter <wolfgang.walter@stusta.net>
-- (c) 2019 Michael Enßlin <mic@sft.lol>
-- (c) 2019-2023 Jonas Jelten <jj@sft.lol>
--

create or replace view active_signing_keys
as
select * from signing_keys
where is_active = true;

create or replace view active_signing_key_data
as
select system_id, key_id, key_type, secret_key, verify_key
from signing_keys
where is_active = true;

create or replace view active_signing_key_pub_data
as
select system_id, key_id, key_type, verify_key
from signing_keys
where is_active = true;
