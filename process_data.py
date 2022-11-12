#!/usr/bin/env python3
import argparse
import os
import sys
import time

import serial

cli = argparse.ArgumentParser()
cli.add_argument('--serial-port')
cli.add_argument('--baud-rate', type=int, default=115200)
cli.add_argument('--out-file')
args = cli.parse_args()

print(args.serial_port)
if args.serial_port is None:
    # auto-determine from lsusb
    USB_DEVICES = '/sys/bus/usb/devices'

    for device in os.listdir(USB_DEVICES):
        try:
            with open(os.path.join(USB_DEVICES, device, 'interface')) as interface_file:
                if interface_file.read().strip().lower() == 'black magic uart port':
                    break
        except FileNotFoundError:
            continue
    else:
        print('BMP not found in USB devices')
        os.execvp('lsusb', ['lsusb'])

    ttys = os.listdir(os.path.join(USB_DEVICES, device, 'tty'))
    if len(ttys) != 1:
        print(f'Unexpected interfaces in USB device {device!r}: {ttys}')
        os.execvp('lsusb', ['lsusb'])

    args.serial_port = os.path.join('/dev', ttys[0])


if args.out_file is not None:
    outfile = open(args.out_file, 'w')
else:
    outfile = None

def convert_ticks_to_volts(ticks):
    return ticks/2**12 * 3.3

def convert_to_amps(volts):
    # we measure half because of averaging of isensa and isensb thus *2
    return volts*10/20*2


port = serial.Serial(args.serial_port,args.baud_rate)
rxbin = []
while True:
    next_byte = port.read(1)
    if (next_byte[0] & 0b11000000 == 0b11000000):
        rxbin.append(next_byte[0] & 0b00111111)
        if len(rxbin) == 2:
            rxdata = rxbin[0] | (rxbin[1] << 6)
            if outfile is not None:
                data = convert_to_amps(convert_ticks_to_volts(rxdata))
                outfile.write(f'{time.monotonic()},{data}\n')
                outfile.flush()
            rxbin.clear()
    else:
        rxbin.clear()
        sys.stdout.buffer.write(next_byte)
        sys.stdout.flush()

port.close()

