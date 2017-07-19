#!/bin/bash
target='xtrlock -l';
who | awk '{print $1, $NF}' | tr -d "()" |
while read u d; do
        id=$(id -u $u)
        . /run/user/$id/dbus-session
        export DBUS_SESSION_BUS_ADDRESS
        export DISPLAY=$d
        sudo su $u -c "$target"

done
