#!/usr/bin/python3

import argparse
import datetime
import struct
import libnacl
import base64
import pyqrcode
import json

HMAC_SIZE = 16
DT_FMT = '%Y-%m-%d %H:%M:%S %Z'

utf8d = [
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, # 00..1f
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, # 20..3f
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, # 40..5f
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, # 60..7f
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9, # 80..9f
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7, # a0..bf
        8,8,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, # c0..df
        0xa,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x4,0x3,0x3, # e0..ef
        0xb,0x6,0x6,0x6,0x5,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8, # f0..ff
        0x0,0x1,0x2,0x3,0x5,0x8,0x7,0x1,0x1,0x1,0x4,0x6,0x1,0x1,0x1,0x1, # s0..s0
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,0,1,0,1,1,1,1,1,1, # s1..s2
        1,2,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1, # s3..s4
        1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,3,1,3,1,1,1,1,1,1, # s5..s6
        1,3,1,1,1,1,1,3,1,3,1,1,1,1,1,1,1,3,1,1,1,1,1,1,1,1,1,1,1,1,1,1, # s7..s8
        ]

def utf8_decode_byte(state, codep, byte):
    type = utf8d[byte]
    if state != 0:
        codep = (byte & 0x3f) | (code << 6)
    else:
        codep = (0xff >> type) & byte;
    state = utf8d[256 + state*16 + type];
    return (state, codep)


def utf8_decode_codep(bs, i):
    state = 0;
    codep = 0;
    try:
        while True:
            state, codep = utf8_decode_byte(state, codep, bs[i])
            i += 1
            if state == 0:
                return (True, i, codep)
            elif state == 1:
                return (False, i, None)
    except IndexError:
        return (False, i, None)


def utf8_decode_char(bs, i):
    (ok, i, codep) = utf8_decode_codep(bs, i)
    if not ok:
        return (False, i, None)
    return (True, i, chr(codep))


def utf8_decode_cstring(bs, i):
    s = ""
    while bs[i] != 0:
        (ok, i, c) = utf8_decode_char(bs, i)
        if not ok:
            return (False, i, None)
        s += c
    return (True, i, s)



def format_datetime(value):
    dt = datetime.datetime.fromtimestamp(value, tz=datetime.timezone.utc)
    return dt.astimezone().strftime(DT_FMT)


def check_msg(cli, args):
    verify_key = bytes.fromhex(args.verify_key_hex.removeprefix("\\x"));
    signed_message = bytes.fromhex(args.message_hex.removeprefix("\\x"));
    signed_message_b64 = base64.b64encode(signed_message).decode();
    message = libnacl.crypto_sign_open(signed_message, verify_key);

    valid_from_min, valid_iv_min = struct.unpack("<IH", message[0:6])
    valid_from = valid_from_min * 60;

    ok, i, system_id = utf8_decode_char(message, 4+2);
    ok, i, key_id = utf8_decode_char(message, i);
    ok, i, message_type = utf8_decode_char(message, i);

    print(f"verifyer key:          {bytes.hex(verify_key)}");
    print(f"signed_message:        {bytes.hex(signed_message)}");
    print(f"signed_message (b64):  {signed_message_b64}");
    print(f"message:               {bytes.hex(message)}");

    print(f"valid from:            {format_datetime(valid_from)} [{valid_from}]")
    print(f"valid for:             {valid_iv_min*60}s")
    print(f"signing system:        {system_id}")
    print(f"signing key_id:        {key_id}")
    print(f"type:                  {message_type}")
    if message_type == "O":
        (ok, i, doorlist) = utf8_decode_cstring(message, i)
        print(f"doorlist:              {doorlist}")
    elif message_type == "F":
        ok, i, keep_key_system_id = utf8_decode_char(message, i)
        ok, i, keep_key_id = utf8_decode_char(message, i)
        ok, i, doorlist = utf8_decode_cstring(message, i)
        print(f"key to keep (system):  {keep_key_system_id}")
        print(f"key to keep (id):      {keep_key_id}")
        print(f"doorlist:              {list(doorlist)}")
    elif message_type == "U":
        ok, i, add_key_system_id = utf8_decode_char(message, i)
        ok, i, add_key_id = utf8_decode_char(message, i)
        ok, i, add_key_type = utf8_decode_char(message, i)
        add_key = message[i:i+32]
        ok, i, doorlist = utf8_decode_cstring(message, i+32)
        doorlist = list(doorlist)
        d1 = [ doorlist[i] for i in range(0, len(doorlist), 2) ]
        d2 = [ doorlist[i] for i in range(1, len(doorlist), 2) ]
        print(f"key to add (system):   {add_key_system_id}")
        print(f"key to add (id):       {add_key_id}")
        print(f"key to add (type):     {add_key_type}")
        print(f"key to add (hex):      {bytes.hex(add_key)}")
        print(f"doorlist:              {doorlist}")
        print(f"doorlist (key system): {d1}")
        print(f"doorlist (owner):      {d2}")
    elif message_type == "I":
        ok, i, p_owner_system_id = utf8_decode_char(message, i)
        ok, i, p_owner_key_id = utf8_decode_char(message, i)
        ok, i, p_owner_key_type = utf8_decode_char(message, i)
        owner_key = message[i:i+32]
        ok, i, door = utf8_decode_char(message, i+32)
        print(f"door owner (system):   {p_owner_system_id}")
        print(f"door owner key id:     {p_owner_key_id}")
        print(f"door owner key type:   {p_owner_key_type}")
        print(f"door owner key (hex):  {bytes.hex(owner_key)}")
        print(f"door:                  {door}")

    if args.qr:
        qrcode = pyqrcode.create(signed_message_b64);
        print(qrcode.terminal());

def check_token(cli, args):
    token = json.loads(args.token)
    msg = base64.b64decode(token['token']).hex()
    valid_until = token['valid_until']
    arglist = [ "check-msg", args.verify_key_hex, msg ]
    if args.qr:
        arglist.insert(0, "--qr")
    args = cli.parse_args(arglist)
    check_msg(cli, args)


def main():
    cli = argparse.ArgumentParser()
    cli.add_argument("--qr", action="store_true")
    subclis = cli.add_subparsers()

    subcli_check_msg = subclis.add_parser("check-msg")
    subcli_check_msg.add_argument("verify_key_hex")
    subcli_check_msg.add_argument("message_hex")
    subcli_check_msg.set_defaults(func=check_msg)

    subcli_check_token = subclis.add_parser("check-token")
    subcli_check_token.add_argument("verify_key_hex")
    subcli_check_token.add_argument("token")
    subcli_check_token.set_defaults(func=check_token)

    args = cli.parse_args()
    args.func(cli, args)


if __name__ == '__main__':
    main()
