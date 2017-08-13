
Frontend for Punting Server
===========================


Install
-------

```sh
npm install
./index.js -h
```

Usage
-----

```
usage: index.js [-h] [-b HOST] [-p PORT] [-r REFRESH_DELAY] [node [node ...]]

Frontend for Punting Server (paiv/puntd)

Positional arguments:
  node                  Nodes to monitor

Optional arguments:
  -h, --help            Show this help message and exit.
  -b HOST, --host HOST, --bind-address HOST
                        Listen address, 0.0.0.0:8080
  -p PORT, --port PORT, --bind-port PORT
                        Listen port
  -r REFRESH_DELAY, --refresh-delay REFRESH_DELAY
                        Refresh period, 15000 millis
```
