#!/usr/bin/env python3

import random
import subprocess

def randbytes(count):
    with open('/dev/urandom', 'rb') as randfile:
        return randfile.read(count)

def main():
    for bytecount in list(range(1024)) + [2**x for x in range(11, 21)]:
        data = randbytes(bytecount)
        a = subprocess.check_output(['./sha256test'], input=data)[:-1]
        b = subprocess.check_output(['sha256sum'], input=data)[:-4]

        if a != b:
            print("sha256 wrong at bytecount %d" % (bytecount,))
            print(subprocess.check_output(['xxd'], input=data).decode())
            print(a)
            print(b)
            return 1

        a = subprocess.check_output(['base64'], input=data).strip()
        b = subprocess.check_output(['./base64test'], input=a)

        if b != data:
            print("base64 wrong at bytecount %d\n" % (bytecount,))
            print(subprocess.check_output(['xxd'], input=data).decode())
            print(subprocess.check_output(['xxd'], input=b).decode())
            return 2
    return 0

if __name__ == '__main__':
    raise SystemExit(main())
