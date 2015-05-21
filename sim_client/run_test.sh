#!/bin/sh

while((1))
do
client.exe &
sleep 60
kill -QUIT `ps -ef|grep client.exe|grep -v grep|grep -v gdb|awk '{print $2}'`
sleep 3
echo '*********************** begin ********************'
done
