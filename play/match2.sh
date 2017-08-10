#!/bin/sh
set -e

trap "exit" INT TERM
trap "kill 0" EXIT

MYDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

HOST="${HOST:-127.0.0.1}"
PORT="${PORT:-9000}"

CLIENT1="${1:-$MYDIR/../code/online.py}"
CLIENT2="${2:-$MYDIR/../code/online.py}"


PLAYERS=2 ./server-ext-all.sh &

LOOP=0
until nc -z "$HOST" "$PORT"
do
    if [ $((LOOP++)) -ge 20 ]; then exit 1; fi
    sleep 0.2
done


"$CLIENT1" --name player1 --host "$HOST" --port "$PORT" --log client1.log &
"$CLIENT2" --name player2 --host "$HOST" --port "$PORT" --log client2.log
