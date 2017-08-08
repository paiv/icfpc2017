
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
usage: gva.py [-h] [-f F] [-l] [-n N] [-p P] [-s] [logfile] [target]

Graph visualizer animated

positional arguments:
  logfile               client log file
  target                target gif file

optional arguments:
  -h, --help            show this help message and exit
  -f F, --fps F         animation frames per second
  -l, --loop            loop animation
  -n N, --max-frames N  stop generating after N frames
  -p P, --save-frames P
                        directory to save frame images
  -s, --silent          be quiet
```
