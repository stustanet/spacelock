-- spacelock database functions
--
-- (c) 2019-2024 Wolfgang Walter <wolfgang.walter@stusta.net>
-- (c) 2019 Michael Enßlin <mic@sft.lol>
-- (c) 2019-2023 Jonas Jelten <jj@sft.lol>
--

-- ----
-- ---- function which must be added by a superuser
-- ---- because they use plpython3u
-- ----

--
-- given a secret create a signing key and return secret and verify-key
-- if the secret is an empty bytestring or is None than create a radnom secret
--
create or replace function create_signing_key_pair(
	p_type char(1),
	out secret_key bytea,
	out verify_key bytea
) as $$
	import libnacl

	if p_type != '1':
		return

	verify_key, secret_key = libnacl.crypto_sign_keypair()
	return secret_key, verify_key
$$ language plpython3u
   set search_path = "$user", public;

--
--  sign a message
--
create or replace function sign_message(
	p_system_id char(1),
	p_key_id char(1),
	p_key_type char(1),
	p_secret_key bytea,
	p_payload bytea,
	p_now_timestamp double precision,
	p_validity_window_size_sec int
) returns bytea as $$
	import struct
	import libnacl

	if p_key_type != '1':
		return None

	message = struct.pack(
		'<IH',
		int(p_now_timestamp/60),
		int(p_validity_window_size_sec/60),
	) + p_system_id.encode() + p_key_id.encode() + p_payload
	signed_message = libnacl.crypto_sign(message, p_secret_key);
	return signed_message
$$ language plpython3u
   set search_path = "$user", public;


