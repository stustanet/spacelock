#!/usr/bin/env python3

from datetime import datetime
import random
import subprocess

def gregtest():
    test_dates = [
        '2000-01-01',
        '2000-01-02',
        '2000-02-01',
        '2000-03-01',
        '2001-03-01',
        '2002-03-01',
        '2003-03-01',
        '2004-03-01',
        '2005-03-01',
        '2006-03-01',
        '2007-03-01',
        '2008-03-01',
        '2009-03-01',
        '2094-03-01',
        '2095-03-01',
        '2096-03-01',
        '2097-03-01',
        '2098-03-01',
        '2099-03-01',
        '2100-03-01',
        '2101-03-01',
        '2102-03-01',
        '2103-03-01',
        '2104-03-01',
        '2105-03-01',
        '2106-03-01',
        '2199-03-01',
        '2200-03-01',
        '2201-03-01',
        '2202-03-01',
        '2203-03-01',
        '2204-03-01',
        '2205-03-01',
        '2299-03-01',
        '2300-03-01',
        '2301-03-01',
        '2399-03-01',
        '2400-03-01',
        '2401-03-01',
        '2402-03-01',
        '2402-04-01',
        '2402-05-01',
        '2402-06-01',
        '2402-07-01',
        '2402-08-01',
        '2402-09-01',
        '2402-10-01',
        '2402-11-01',
        '2402-12-01',
        '2402-12-20',
        '2402-12-30',
        '2402-12-31',
        '2403-01-01',
        '2404-01-31',
        '2404-02-29',
        '2404-03-31',
        '2404-04-30',
        '2404-05-31',
        '2404-06-30',
        '2404-07-31',
        '2404-08-31',
        '2404-09-30',
        '2404-10-31',
        '2404-11-30',
        '2404-12-31',
        '9999-10-10',
    ]

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
