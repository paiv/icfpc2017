
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
usage: puntd.js [-h] [-b HOST] [-p PORT] [-m MAP] [-n PLAYERS] [-f] [-o] [-s]
                [-th HANDSHAKE] [-ts SETUP] [-tm MOVE]


Punter Server (paiv)

Optional arguments:
  -h, --help            Show this help message and exit.
  -b HOST, --host HOST, --bind-address HOST
                        Listen address, localhost:9000
  -p PORT, --port PORT, --bind-port PORT
                        Listen port
  -m MAP, --map MAP     Game map (maps/sample.json)
  -n PLAYERS, --players PLAYERS
                        Number of players, 2
  -f, --futures         Enable futures
  -o, --options         Enable options
  -s, --splurges        Enable splurges
  -th HANDSHAKE, --handshake-timeout HANDSHAKE
                        Player timeout on handshake (until "me"), 1 sec
  -ts SETUP, --setup-timeout SETUP
                        Player timeout on setup (until "ready"), 10 sec
  -tm MOVE, --move-timeout MOVE
                        Player timeout on each move (until response received
                        fully), 1 sec
```
