
Generate animated gifs from play logs.

    ./gva.py client.log output.gif

Needs `Graphviz` and `ImageMagick` installed (`dot` and `convert` command line).

Logfiles are json objects per line, as they sent/received by offline punter client
(without n: prefix).

    {"me": "bot"}
    {"you": "bot"}
    {"map": {...
    {"ready": {...
    ...


Usage
-----

```
usage: gva.py [-h] [-f FPS] [-l] [-n LIMIT] [-p DIR] [--server] [-s]
              [logfile] [target]

Graph visualizer animated

positional arguments:
  logfile               source log file
  target                target gif file

optional arguments:
  -h, --help            show this help message and exit
  -f FPS, --fps FPS     animation frames per second
  -l, --loop            loop animation
  -n LIMIT, --max-frames LIMIT
                        stop generating after N frames
  -p DIR, --save-frames DIR
                        directory to save frame images
  --server              server log format
  -s, --silent          be quiet
```
