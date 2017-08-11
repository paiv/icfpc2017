#!/usr/bin/env python3
import json
import os
import shutil
import subprocess
import tempfile

from glob import glob
from graphviz import dot
from imagemagick import ImageMagick
from operator import itemgetter


class LogFormat:
    AUTO = 0
    SERVER = 1
    CLIENT = 2


class GameBoard:
    def __init__(self):
        self.graph = dot.Graph(directed=False)

        self.graph.node_attr['label'] = ''
        self.graph.node_attr['shape'] = 'point'
        self.graph.node_attr['color'] = 'grey40'

        self.graph.edge_attr['style'] = 'dotted'
        self.graph.edge_attr['color'] = 'grey40'
        self.graph.edge_attr['arrowhead'] = 'none'
        self.graph.edge_attr['arrowtail'] = 'none'


        self.site_attrs = {
            'shape': 'point',
            'color': 'grey40',
        }

        self.mine_attrs = {
            'shape': 'circle',
            'color': 'firebrick1',
            'fillcolor': 'firebrick1',
            'style': 'filled',
            'width': '0.1',
            'height': '0.1',
        }

        self.claimed_edge_attrs = {
            'style': 'bold',
        }

        self.my_color = 'red'

        self.player_colors = [
            'dodgerblue3',
            'coral4',
            'forestgreen',
            'gray16',
            'darkorchid2',
            'goldenrod1',
            'yellow3',
            'brown1',
            'limegreen',
            'purple1',
            'greenyellow',
            'gray32',
            'orange2',
            'magenta',
            'darkgreen',
            'gray48',
        ]

    def setup(self, player_id, players, mapobj):
        self.player_id = player_id
        self.players = players
        self.claims = set()

        mines = mapobj['mines']
        sites = mapobj['sites']
        rivers = mapobj['rivers']

        sites = [(site['id'], site['x'], -site['y']) for site in sites]

        minx = min(x for _,x,y in sites)
        maxx = max(x for _,x,y in sites)
        miny = min(y for _,x,y in sites)
        maxy = max(y for _,x,y in sites)
        dx = maxx - minx
        dy = maxy - miny

        scale = None
        w = max(dx, dy)

        if w > 12:
            scale = int(round(w / 12))
        elif w < 2:
            scale = 0.1

        if scale:
            self.graph.graph_attr['inputscale'] = scale

        for (id,x,y) in sites:
            pos = '{},{}!'.format(x, y)
            mine = id in mines
            attrs = self.mine_attrs if mine else dict()
            self.graph.node(id, pos=pos, **attrs)

        for river in rivers:
            x, y = self._enorm(river['source'], river['target'])
            self.graph.edge(x, y)

    def _enorm(self, x, y=None):
        if y is None:
            x, y = x
        if x > y:
            x, y = y, x
        return x, y

    def claim(self, player_id, river):
        river = self._enorm(river)
        self.claims.add(river)

        attrs = self.claimed_edge_attrs.copy()
        attrs['color'] = self._player_color(player_id)

        self.graph.update_edge(river, **attrs)

    def option(self, player_id, river):
        river = self._enorm(river)

        attrs = self.claimed_edge_attrs.copy()
        option_color = self._player_color(player_id)
        claim_color = self.graph.get_edge_attrs(river)['color']

        attrs['dir'] = 'both'
        attrs['color'] = '{}:{}'.format(claim_color, option_color)

        self.graph.update_edge(river, **attrs)

    def splurge(self, player_id, route):
        for river in zip(route, route[1:]):
            river = self._enorm(river)

            if river in self.claims:
                self.option(player_id, river)
            else:
                self.claim(player_id, river)

    def _player_color(self, player):
        return (self.my_color if player == self.player_id
            else self.player_colors[player % len(self.player_colors)])


class LogAnimator:

    def __init__(self, fps=1.0, loop=False, save_frames=None, max_frames=None, log_format=None, silent=False):
        self.fps = fps
        self.loop = loop
        self.save_frames = save_frames
        self.max_frames = max_frames or 0
        self.log_format = log_format or LogFormat.AUTO
        self.silent = silent
        self.frame_count = 0

    def process(self, logfile, target):

        if hasattr(logfile, 'read'):
            self._process_file(logfile, target)
        else:
            with open(logfile, 'r') as fd:
                self._process_file(fd, target, title=logfile)

    def _process_file(self, infile, target, title=None):
        with tempfile.TemporaryDirectory() as tempdir:
            frame_file = os.path.join(tempdir, 'frame.png')

            self._generate_frames(infile, frame_file)
            self._animate(tempdir, target)

            if self.save_frames:
                self._move_files(tempdir, self.save_frames)

    def _move_files(self, source_dir, target_dir):
        os.makedirs(target_dir, exist_ok=True)
        for fn in glob(os.path.join(source_dir, '*')):
            shutil.move(fn, target_dir)

    def _generate_frames(self, infile, target):
        STATE_HANDSHAKE = 0
        STATE_SETUP = 1
        STATE_GAMEPLAY = 2
        STATE_SCORING = 3

        state = STATE_HANDSHAKE
        board = None

        for line in infile.readlines():

            try:
                message = json.loads(line)
            except json.JSONDecodeError:
                continue

            if message.get('stop', None) is not None:
                state = STATE_SCORING

            if state == STATE_HANDSHAKE:
                if message.get('map', None) is not None:
                    state = STATE_SETUP

                    player_id = message['punter']
                    players = message['punters']
                    mapobj = message['map']

                    board = self._setup_board(player_id, players, mapobj, target=target)

            elif state == STATE_SETUP:
                if message.get('ready', None) is not None:
                    state = STATE_GAMEPLAY

                    if self.log_format == LogFormat.AUTO:
                        self.log_format = LogFormat.CLIENT

                elif message.get('start', None) is not None:
                    state = STATE_GAMEPLAY

                    if self.log_format == LogFormat.AUTO:
                        self.log_format = LogFormat.SERVER

            elif state == STATE_GAMEPLAY:
                moves = None

                if self.log_format == LogFormat.SERVER:
                    claim = message.get('claim', None)
                    option = message.get('option', None)
                    splurge = message.get('splurge', None)

                    move = claim or option or splurge
                    if move is not None:
                        moves = [message]

                elif self.log_format == LogFormat.CLIENT:
                    move = message.get('move', None)
                    if move is not None:
                        moves = move['moves']

                if moves is not None:
                    self._update_board(board, moves, target=target)

            elif state == STATE_SCORING:
                stop = message.get('stop')
                moves = stop.get('moves', None)
                if moves is not None:
                    self._update_board(board, moves, target=target)

    def _setup_board(self, player_id, players, obj, title=None, target=None):
        board = GameBoard()
        board.setup(player_id, players, obj)

        self._export_frame(board, target=target)

        return board

    def _update_board(self, board, moves, target=None):
        # import pdb; pdb.set_trace()
        moves = [(next(iter(x.values())).get('punter', -1), x) for x in moves]
        moves = sorted(moves, key=itemgetter(0))
        l = [x for k,x in moves if k < board.player_id]
        r = [x for k,x in moves if k >= board.player_id]
        moves = r + l

        for move in moves:
            claim = move.get('claim', None)
            option = move.get('option', None)
            splurge = move.get('splurge', None)

            if claim is not None:
                board.claim(claim['punter'], (claim['source'], claim['target']))
            elif option is not None:
                board.option(option['punter'], (option['source'], option['target']))
            elif splurge is not None:
                board.splurge(splurge['punter'], splurge['route'])

            self._export_frame(board, target)

    def _export_frame(self, board, target):
        if self.max_frames > 0 and self.frame_count >= self.max_frames:
            return

        frameno = self.frame_count
        self.frame_count += 1

        base,ext = os.path.splitext(target)
        fn = '{}{:06}{}'.format(base, frameno, ext)

        if not self.silent:
            sys.stderr.write('.')
            sys.stderr.flush()

        plotter = dot.Plotter()
        plotter.plot(board.graph, fn, format='png')

    def _animate(self, from_dir, target):
        if not self.silent:
            sys.stderr.write('♡\n')
            sys.stderr.flush()

        delay = round(10000 / self.fps) / 100
        loop = 0 if self.loop else 1

        img = ImageMagick(layers='optimize', alpha='on')
        img.animate(from_dir, target, loop=loop, delay=delay)


def do_viz(logfile, target, **kwargs):
    anim = LogAnimator(**kwargs)
    anim.process(logfile, target)


if __name__ == '__main__':

    import argparse
    import sys


    parser = argparse.ArgumentParser(description='Graph visualizer animated')

    parser.add_argument('logfile', nargs='?', type=argparse.FileType(mode='r'),
        default=sys.stdin,
        help='source log file')

    parser.add_argument('target', nargs='?', type=argparse.FileType(mode='w'),
        default=sys.stdout.buffer,
        help='target gif file')

    parser.add_argument('-f', '--fps', metavar='FPS', type=float, default=2.0,
        help='animation frames per second')

    parser.add_argument('-l', '--loop', action='store_true',
        help='loop animation')

    parser.add_argument('-n', '--max-frames', metavar='LIMIT', type=int, default=None,
        help='stop generating after N frames')

    parser.add_argument('-p', '--save-frames', metavar='DIR', type=str, default=None,
        help='directory to save frame images')

    parser.add_argument('--server', action='store_true',
        help='server log format')

    parser.add_argument('-s', '--silent', action='store_true',
        help='be quiet')

    args = parser.parse_args()


    log_format = LogFormat.SERVER if args.server else LogFormat.AUTO


    do_viz(
        logfile=args.logfile,
        target=args.target,
        fps=args.fps,
        loop=args.loop,
        save_frames=args.save_frames,
        max_frames=args.max_frames,
        log_format=log_format,
        silent=args.silent,
        )
