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


class GameBoard:
    def __init__(self):
        # handle claim and option on single edge
        self.graph = dot.Graph(directed=True)

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
            'style': 'solid',
        }

        self.my_color = 'red'

        self.player_colors = [
            'dodgerblue3',
            # 'deeppink1',
            'forestgreen',
            'gray16',
            'darkorchid2',
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

        sites = [(site['id'], site['x'], site['y']) for site in sites]

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

    def __init__(self, fps=1.0, loop=False, save_frames=None, limit=None, silent=False):
        self.fps = fps
        self.loop = loop
        self.save_frames = save_frames
        self.max_frames = limit or 0
        self.silent = silent
        self.frame_count = 0
        self.max_frames = 0

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
                    player_id = message['punter']
                    players = message['punters']
                    mapobj = message['map']

                    board = self._setup_board(player_id, players, mapobj)

                    state = STATE_SETUP

            elif state == STATE_SETUP:
                if message.get('ready', None) is not None:
                    self._export_frame(board, target)
                    state = STATE_GAMEPLAY

            elif state == STATE_GAMEPLAY:
                move = message.get('move', None)
                if move is not None:
                    self._update_board(board, move['moves'], target)

            elif state == STATE_SCORING:
                stop = message.get('stop')
                moves = stop['moves']
                self._update_board(board, moves, target)

    def _setup_board(self, player_id, players, obj, title=None):
        board = GameBoard()
        board.setup(player_id, players, obj)

        return board

    def _update_board(self, board, moves, target):
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


def do_viz(logfile, target, fps, loop, save_frames, max_frames, silent):
    anim = LogAnimator(fps=fps, loop=loop, save_frames=save_frames, limit=max_frames, silent=silent)
    anim.process(logfile, target)


if __name__ == '__main__':

    import argparse
    import sys


    parser = argparse.ArgumentParser(description='Graph visualizer animated')

    parser.add_argument('logfile', nargs='?', type=argparse.FileType(mode='r'),
        default=sys.stdin,
        help='client log file')

    parser.add_argument('target', nargs='?', type=argparse.FileType(mode='w'),
        default=sys.stdout.buffer,
        help='target gif file')

    parser.add_argument('-f', '--fps', metavar='F', type=float, default=2.0,
        help='animation frames per second')

    parser.add_argument('-l', '--loop', action='store_true',
        help='loop animation')

    parser.add_argument('-n', '--max-frames', metavar='N', type=int, default=None,
        help='stop generating after N frames')

    parser.add_argument('-p', '--save-frames', metavar='P', type=str, default=None,
        help='directory to save frame images')

    parser.add_argument('-s', '--silent', action='store_true',
        help='be quiet')

    args = parser.parse_args()

    do_viz(
        logfile=args.logfile,
        target=args.target,
        fps=args.fps,
        loop=args.loop,
        save_frames=args.save_frames,
        max_frames=args.max_frames,
        silent=args.silent,
        )
