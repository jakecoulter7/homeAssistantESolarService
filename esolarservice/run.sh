#!/bin/sh

cat >/tmp/esolard.conf <<EOF
inverter_host = 192.168.68.110
poll_seconds = 15
http_port = 80
EOF

exec ./esolard -c /tmp/esolard.conf
