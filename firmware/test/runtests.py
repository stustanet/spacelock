#!/usr/bin/env python3

from datetime import datetime
import random
import subprocess

def dcf77test():
    # data provided by dcf77logs.de
    with open('dcf77testdata') as fileobj:
        lines = fileobj.read().split('\n')[:-1]

    timestamps = []
    bitstrings = []

    for line in lines:
        line = line.strip()
        if not line:
            continue

        bits, timestring, timezone = line.split(', ')

        bits = bits.split('  ')[0]
        if timezone == 'SZ':
            timezone = '+0200'
        else:
            timezone = '+0100'

        dt = datetime.strptime(timestring + ' ' + timezone, '%d.%m.%y %H:%M:%S %z')

        timestamps.append(int(dt.timestamp()))
        bitstrings.append(bits)

    results = subprocess.check_output(['./dcf77test'], input=('\n'.join(bitstrings)+'\n').encode())
    results = [int(x) for x in results.decode().split('\n')[:-1]]

    if results != timestamps:
        print((results, timestamps))

def gregtest():
    with open('gregoriancalendartestdata') as fileobj:
        test_dates = fileobj.read().split('\n')[:-1]

    text = ('\n'.join(test_dates) + '\n').encode()
    results = subprocess.check_output(['./gregoriancalendartest'], input=text)
    for idx, result in enumerate(results.decode().split('\n')[:-1]):
        test_date = datetime.fromisoformat(test_dates[idx] + ' 00:00:00+00:00')
        res_doy, res_dow, res_ts = [int(x, 10) for x in result.split()]
        exp_doy = test_date.timetuple().tm_yday
        exp_dow = test_date.isoweekday()
        exp_ts = int(test_date.timestamp())

        if res_doy != exp_doy or res_dow != exp_dow or res_ts != exp_ts:
            print((test_date, exp_doy, res_doy, exp_dow, res_dow, exp_ts, res_ts))

def randbytes(count):
    with open('/dev/urandom', 'rb') as randfile:
        return randfile.read(count)

def main():
    gregtest()
    dcf77test()

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
