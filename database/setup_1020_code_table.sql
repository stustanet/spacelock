-- spacelock database functions
--
-- (c) 2019-2024 Wolfgang Walter <wolfgang.walter@stusta.net>
-- (c) 2019 Michael Enßlin <mic@sft.lol>
-- (c) 2019-2023 Jonas Jelten <jj@sft.lol>
--

--
-- function which must exist at table creation time
--

--
--  generate a random bigint
--
create or replace function gen_random_bigint (
) returns bigint as $$
declare
	bytes bytea;
	rnd bigint := 0;
begin
	bytes = gen_random_bytes(8);
	
	for i in 0 .. 7 loop
		rnd = (rnd << 8) | get_byte(bytes, i);
	end loop;
	return abs(rnd);
end;
$$ language plpgsql
   set search_path = "$user", public;

