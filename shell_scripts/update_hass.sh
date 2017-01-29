#!/bin/bash

# Run backup first
/home/homeassistant/.homeassistant/shell_scripts/backup_config.sh

# Run update
pushd /home/homeassistant >/dev/null
sudo pip3 install --upgrade homeassistant
hassctl restart
popd >/dev/null