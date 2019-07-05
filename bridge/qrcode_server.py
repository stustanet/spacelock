#!/usr/bin/env python2

import argparse
import base64
import hashlib
import pprint
import subprocess

import cbor
import zbar

cli = argparse.ArgumentParser()
cli.add_argument('device')
cli.add_argument('--debug', action='store_true')
args = cli.parse_args()

# create a Processor
proc = zbar.Processor()

# configure the Processor
# proc.parse_config('enable')
proc.parse_config('disable')
proc.parse_config('qrcode.enable')

# initialize the Processor
proc.init(args.device, enable_display=args.debug)

if args.debug:
    proc.visible = True

def send(text):
    subprocess.call(['curl', 'http://localhost:8000/send/' + text])

# read at least one barcode (or until window closed)
while True:
    proc.process_one()
    for symbol in proc.results:
        message = symbol.data
        print("Message:\n%s" % (pprint.pformat(message),))
        send(message)

