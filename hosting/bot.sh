#!/bin/bash

trap "exit" INT TERM
trap "kill 0" EXIT

MYDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

HOST="TEMPLATE_HOST"
PORT="${1:-9001}"
CMD="${2:-./punter}"

while [ 1 ]
do

  until nc -z "$HOST" "$PORT"
  do
    sleep 1
  done 2> /dev/null

  "$MYDIR/lamduct-0.3" --log-level 0 --game-hostname "$HOST" --game-port "$PORT" "$CMD" 2>/dev/null
done
