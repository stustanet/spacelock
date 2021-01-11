import argparse
import base64
import binascii
import datetime
import struct

HMAC_SIZE = 16
DT_FMT = '%Y-%m-%d %H:%M:%S %Z'


def format_datetime(value):
    dt = datetime.datetime.fromtimestamp(value, tz=datetime.timezone.utc)
    return dt.astimezone().strftime(DT_FMT)

def main():
    cli = argparse.ArgumentParser()
    cli.add_argument('code_base64')
    args = cli.parse_args()

    binary = base64.b64decode(args.code_base64)

    hmac = binary[:HMAC_SIZE]
    print(f'HMAC:        {binascii.hexlify(hmac).decode("ascii")}')

    valid_from, valid_until = struct.unpack_from('<QQ', binary, HMAC_SIZE)
    print(f'valid from:  {format_datetime(valid_from)} [{valid_from}]')
    print(f'valid until: {format_datetime(valid_until)} [{valid_until}]')

    user_id = binary[HMAC_SIZE + 2 * 8:].decode('ascii', errors='replace')
    print(f'User ID:     {user_id}')

if __name__ == '__main__':
    main()
