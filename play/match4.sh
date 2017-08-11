#!/bin/sh
set -e

trap "exit" INT TERM
trap "kill 0" EXIT

MYDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

HOST="${HOST:-127.0.0.1}"
PORT="${PORT:-9000}"

CLIENT1="${1:-$MYDIR/../code/online.py}"
CLIENT2="${2:-$MYDIR/../code/online.py}"
CLIENT3="${1:-$MYDIR/../code/online.py}"
CLIENT4="${1:-$MYDIR/../code/online.py}"


PLAYERS=4 ./server-ext-all.sh &
SERVER_PID=$(jobs -p)

LOOP=0
until nc -z "$HOST" "$PORT"
do
    if [ $((LOOP++)) -ge 20 ]; then exit 1; fi
    sleep 0.2
done 2> /dev/null


"$CLIENT1" -s --name player1 --host "$HOST" --port "$PORT" --log client1.log &
"$CLIENT2" -s --name player2 --host "$HOST" --port "$PORT" --log client2.log &
"$CLIENT3" -s --name player3 --host "$HOST" --port "$PORT" --log client3.log &
"$CLIENT4" -s --name player4 --host "$HOST" --port "$PORT" --log client4.log

wait $SERVER_PID
