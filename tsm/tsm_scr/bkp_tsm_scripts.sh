#!/bin/bash

USER=$1
PASS=$2
server=$3

TSM_CMD="/usr/bin/dsmadmc -tab -dataonly=yes -id=$USER -password=$PASS -servername=$server"


SCRIPTS=$($TSM_CMD q scr | awk '{print $1}' | grep -i -v compareoffsite)

echo '

' > scripts.t2t

for i in $SCRIPTS; do
#for i in drivepath drmdb; do
$TSM_CMD "q scr $i"  | sed 's/^/- /' | sed 's/\t$//' | sed 's/\t\(.*\)/ - Desc="\1"/' >> scripts.t2t
echo '


```' >> scripts.t2t
$TSM_CMD "q scr $i f=raw"  >> scripts.t2t
echo '```' >> scripts.t2t
echo >> scripts.t2t
done

