#!/bin/sh
# Builds /etc/esolard.conf from the add-on options (Home Assistant > the add-on > Configuration),
# then starts esolard. Passwords and your exact location stay in Home Assistant, not in this repo.
set -eu

OPTIONS=/data/options.json
CONF=/etc/esolard.conf

# value of one option; empty if it is missing or null
opt() { jq -r --arg k "$1" '.[$k] | if . == null then "" else tostring end' "$OPTIONS"; }

LAT=$(opt latitude)
LON=$(opt longitude)
SQL=$(opt sql_active)

umask 077
{
    echo "inverter_host = $(opt inverter_host)"
    echo "inverter_port = 1502"
    echo "poll_seconds = $(opt poll_seconds)"
    echo "http_port = 80"
    echo "http_bind = 0.0.0.0"
    echo "hist_keep_days = 366"
    echo "timezone = $(opt timezone)"
    if [ -n "$LAT" ]; then echo "latitude = $LAT"; fi
    if [ -n "$LON" ]; then echo "longitude = $LON"; fi
    if [ "$SQL" = "true" ]; then
        echo "sql_active = yes"
        echo "sql_server = $(opt sql_server)"
        echo "sql_port = $(opt sql_port)"
        echo "sql_table = esolaredge.power"
        echo "sql_log_table = esolaredge.powerlog"
        echo "sql_user = $(opt sql_user)"
        echo "sql_password = $(opt sql_password)"
        echo "sql_weather = yes"
        echo "sql_daily_split = yes"
        echo "sql_voltage_tol = yes"
    fi
    echo "verbose = no"
} > "$CONF"

echo "Starting esolard, settings from the add-on options, data in /data/esolar"
# -d keeps the chart and day totals in /data, which survives add-on updates
exec /app/esolard -c "$CONF" -d /data/esolar
