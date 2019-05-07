import argparse
from hashlib import sha256

import base64
import cbor
import json
import qrcode

cli = argparse.ArgumentParser()
cli.add_argument("message")
args = cli.parse_args()

message = json.loads(args.message)
encoded = cbor.dumps(message)

with open('secretkey', 'rb') as fileobj:
    secret_key = fileobj.read()

hashobj = sha256(secret_key)
hashobj.update(encoded)

signed = hashobj.digest() + encoded

qrcode.make(base64.b64encode(signed)).show()

print(repr(signed[:32]))
print(repr(signed[32:]))
