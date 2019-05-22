# Microcontroller

A STM32 microcontroller controls the main aspects of the door lock.
It uses

- A DCF77 receiver module to acquire a real-time timestamp
- A stepper motor controller to control the door lock motor
- A UART communication interface to receive commands.

A 32-byte secret key is stored at flash address 0x8000fc00; all incoming commands
are authenticated against this key via HMAC.

The firmware can be found in the `firmware` subfolder.
Build it by changing to the `src` folder and running `make`.
Flash it by running `./gdbserver`, `./debug` and typing `load`.

The microcontroller is a trusted part of the system, since it verifies
the commands and is able to directly control the door lock.
If an attacker can read the secret key, re-write parts of the flash or exploit
the firmware through UART messages, they control the system and can open the
door.

# Communication protocol

The communication protocol of the microcontroller is open.
Messages are sent at 115200 Baud/s 8N1 (3.3V).

When '\0', '\r' or '\n' are received, the previously-received characters are
validated and processed as a single message. The maximum message size is 256 bytes.

IMPORTANT: If the message is 'backdoor', the door will open. Don't tell anybody!

All messages have the following format:

- 16-byte HMAC signature
- 8-byte start-of-validity timestamp (seconds since epoch, `uint64_t LE`)
- 8-byte end-of-validity timestamp (seconds since epoch, `uint64_t LE`)
- 1-byte message type (`uint8_t`)
- 0 or more bytes of message payload (depending on message type)

## HMAC validation

The following pseudocode describes the HMAC calculation method:

```python
import hashlib
hash = hashlib.sha256()
hash.update(SECRET_KEY)
hash.update(message[16:])
hmac = hash.digest()[:16]
```

## Message types

### Message type 0

Open the door. The message payload is the comment, which is there for logging
purposes and usually holds the username.

### Message type 1

Write a new secret key. The message payload is the seed for the new secret key.

The following pseudocode describes the new secret key calculation method:

```python
import hashlib
hash = hashlib.sha256()
hash.update(SECRET_KEY)
hash.update(payload)
NEW_SECRET_KEY = hash.digest()
```
