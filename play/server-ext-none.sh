#!/bin/bash
set -e

trap "exit" INT TERM
trap "kill 0" EXIT


MYDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"


HOST="${HOST:-127.0.0.1}"
PORT="${PORT:-9000}"

SERVER="${SERVER:-$MYDIR/../server/puntd.js}"

MAP="${GAMEMAP:-$MYDIR/maps/sample.json}"
PLAYERS="${PLAYERS:-2}"

"$SERVER" --host "$HOST" --port "$PORT" --players "$PLAYERS" --map "$MAP"
