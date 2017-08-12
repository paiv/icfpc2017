
Punting Server
==============


Install
-------

```sh
npm install
./puntd -h
```

Usage
-----

```
usage: puntd.js [-h] [-b HOST] [-p PORT] [-w MONITOR] [-m MAP] [-n PLAYERS]
                [-f] [-o] [-s] [-th HANDSHAKE_TIMEOUT] [-ts SETUP_TIMEOUT]
                [-tm MOVE_TIMEOUT]


Punter Server (paiv)

Optional arguments:
  -h, --help            Show this help message and exit.
  -b HOST, --host HOST, --bind-address HOST
                        Listen address, 0.0.0.0:9000
  -p PORT, --port PORT, --bind-port PORT
                        Listen port
  -w MONITOR, --monitor-port MONITOR
                        Monitoring port (http), 8080
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


# Docker image

Run
---

```sh
docker run --rm paiv/puntd /app/puntd.js -h
```

#### Bind to local port 9123

Inside docker container, the service binds to port 9000. You need to translate
between local port 9123 and container 9000:

```sh
docker run --rm -p 9123:9000 paiv/puntd
```

#### Change map

To run a map bundled inside container:

```sh
docker run --rm -p 9123:9000 paiv/puntd \
    /app/puntd.js --map maps/circle.json
```

#### Local maps

To run your maps, you can mount them into container:

```sh
docker run --rm -p 9123:9000 \
    -v $PWD/mymaps:/app/mymaps \
    paiv/puntd \
    /app/puntd.js --map mymaps/some.json
```

#### Monitoring

Simple monitoring service runs on port 8080. You might want to expose it too:

```sh
docker run --rm -p 9123:9000 -p 8080:8080 paiv/puntd
```


Build
-----

To build docker image from source:

```sh
docker build -t paiv/puntd .
```
