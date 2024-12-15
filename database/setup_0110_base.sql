-- spacelock database
--
-- (c) 2019-2023 Wolfgang Walter <wolfgang.walter@stusta.net>
-- (c) 2019 Michael Enßlin <mic@sft.lol>
-- (c) 2019-2023 Jonas Jelten <jj@sft.lol>
--

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

