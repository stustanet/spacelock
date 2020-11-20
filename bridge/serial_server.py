#!/usr/bin/env python3

"""
read a serial port for auth messages
send them to the message broker

GPLv3 or later
(c) 2020 Jonas Jelten <jj@sft.lol>
"""

import argparse
import subprocess

import serial

cli = argparse.ArgumentParser()
cli.add_argument('--device', default="/dev/ttyUSB0")
args = cli.parse_args()

print("launching serial bridge...")

serial = serial.Serial(args.device, 9600)

def send(text):
    subprocess.call(['curl', 'http://localhost:8000/send/' + text])

while True:
    message = serial.readline()
    message = message.decode().strip()
    print(f"Message:\n{message!r}")
    send(message)
