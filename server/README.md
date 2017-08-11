
Punting Server
==============


# Install

```sh
npm install
./puntd -h
```

# Usage

```
usage: puntd.js [-h] [-b HOST] [-p PORT] [-m MAP] [-n PLAYERS] [-f] [-o] [-s]
                [-th HANDSHAKE_TIMEOUT] [-ts SETUP_TIMEOUT] [-tm MOVE_TIMEOUT]


Punter Server (paiv)

Optional arguments:
  -h, --help            Show this help message and exit.
  -b HOST, --host HOST, --bind-address HOST
                        Listen address, 0.0.0.0:9000
  -p PORT, --port PORT, --bind-port PORT
                        Listen port
  -m MAP, --map MAP     Game map, maps/sample.json
  -n PLAYERS, --players PLAYERS
                        Number of players, 2
  -f, --futures         Enable futures
  -o, --options         Enable options
  -s, --splurges        Enable splurges
  -th HANDSHAKE_TIMEOUT, --handshake-timeout HANDSHAKE_TIMEOUT
                        Player timeout on handshake (until "me"), 1 sec
  -ts SETUP_TIMEOUT, --setup-timeout SETUP_TIMEOUT
                        Player timeout on setup (until "ready"), 10 sec
  -tm MOVE_TIMEOUT, --move-timeout MOVE_TIMEOUT
                        Player timeout on each move (until response received
                        fully), 1 sec
```


Docker image
------------


# Build

Build the image:

```sh
docker build -t paiv/puntd .
```

# Run

##### Run once, random port

```sh
docker run --rm -P paiv/puntd
```

##### Bind to local port 9123

```sh
docker run --rm -p 9123:9000 paiv/puntd
```

##### Set game map

```sh
docker run -e 'MAP=maps/lambda.json' --rm -p 9123:9000 paiv/puntd
```

Full control

```sh
docker run \
    -e 'MAP=maps/sample.json' \
    -e 'PLAYERS=2' \
    -e 'FUTURES=1' \
    -e 'SPLURGES=1' \
    -e 'OPTIONS=1' \
    -e 'HANDSHAKE_TIMEOUT=1' \
    -e 'SETUP_TIMEOUT=10' \
    -e 'MOVE_TIMEOUT=1' \
    --rm -p 9123:9000 paiv/puntd
```
