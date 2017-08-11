#!/bin/sh
set -e

trap "exit" INT TERM
trap "kill 0" EXIT

MYDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

HOST="${HOST:-127.0.0.1}"
PORT="${PORT:-9000}"

PLAYERS=3
CLIENT1="${1:-$MYDIR/../code/online.py}"
CLIENT2="${2:-$MYDIR/../code/online.py}"
CLIENT3="${1:-$MYDIR/../code/online.py}"

export HOST PORT PLAYERS


./server-ext-all.sh | \
    ( [[ "$LOGFILE" ]] && tee "$LOGFILE" || cat ) &

SERVER_PID=$(jobs -p)


LOOP=0
until nc -z "$HOST" "$PORT"
do
    if [ $((LOOP++)) -ge 20 ]; then exit 1; fi
    sleep 0.2
done 2> /dev/null


"$CLIENT1" --name player1 --host "$HOST" --port "$PORT" -s --no-log &
"$CLIENT2" --name player2 --host "$HOST" --port "$PORT" -s --no-log &
"$CLIENT3" --name player3 --host "$HOST" --port "$PORT" -s --no-log

wait $SERVER_PID
