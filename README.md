# Spacelock

StuStaNet Hackerspace Lock

# Firmware build instructions

The spacelock firmware can be configured for several use cases.

For each use case there is a config file in firmware/build_config.

In order to activate a config, `ln -s` the chosen config file to firmware/src/config.h