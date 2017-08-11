#!/bin/bash
set -e

trap "exit" INT TERM
trap "kill 0" EXIT


MYDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

SERVER="${SERVER:-$MYDIR/../server/puntd.js}"

HOST="${HOST:-127.0.0.1}"
PORT="${PORT:-9000}"
MAP="${MAP:-$MYDIR/maps/sample.json}"
PLAYERS="${PLAYERS:-2}"

export HOST PORT MAP PLAYERS

"$SERVER"
