#!/bin/bash

USER=$1
PASS=$2
server=$3

TSM_CMD="/usr/bin/dsmadmc -tab -dataonly=yes -id=$USER -password=$PASS -servername=$server"

echo '

 || Script name | Description |' > summary.t2t 
$TSM_CMD q scr | grep -v -i compareoffsite | sed 's/\t$//' | sed 's/^/ | /' | sed 's/\t\(.*\)/ | \1 |/' >> summary.t2t

