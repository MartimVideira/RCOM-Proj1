#! /bin/bash
echo "Port1:    /dev/ttyS10"
echo "Port2:    /dev/ttyS11"
sudo socat -d  -d  PTY,link=/dev/ttyS10,mode=777   PTY,link=/dev/ttyS11,mode=777 &
