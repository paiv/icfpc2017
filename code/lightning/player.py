#!/usr/bin/env python3
import io
import json
import os
import random
import sys

from datetime import datetime
from collections import OrderedDict


class Logger:
    def __init__(self, fn=None):
        if fn is None:
            os.makedirs('logs', exist_ok=True)
            now = datetime.now().strftime('%Y%m%d%H%M%S')
            fn = 'logs/%s.log' % now
        self.file = io.open(fn, 'a')
        self.file_name = fn

    def log(self, text, *args):
        self.file.write(text)
        for arg in args:
            self.file.write(str(arg))
        self.file.write('\n')


class PunterTransport:
    def send(self, obj):
        packet = json.dumps(obj, separators=(',', ':'))
        packet = bytes(packet, 'utf-8')
        header = bytes('%d:' % len(packet), 'ascii')
        self.write(header)
        self.write(packet)

    def receive(self):
        # s,_,_ = select.select([self.socket], [], [], 10)

        chunks = []

        total = 20
        body = ''

        while True:
            chunk = self.read(total)
            if chunk == b'':
                raise PunterTransportError()

            chunks.append(chunk)

            data = str(b''.join(chunks), 'utf-8')
            parts = data.split(':', 1)
            if len(parts) == 2:
                body = parts[1]
                total = int(parts[0]) - len(body)
                break

        chunks = []

        while total > 0:
            chunk = self.read(total)
            if chunk == b'':
                raise PunterTransportError()

            chunks.append(chunk)
            total -= len(chunk)

        response = b''.join(chunks)
        response = body + str(response, 'utf-8')
        response = json.loads(response, object_pairs_hook=OrderedDict)
        return response

    def close(self):
        pass

    def __enter__(self):
        pass

    def __exit__(self, exc_type, exc_value, traceback):
        self.close()


class PunterFileTransport(PunterTransport):
    def __init__(self, rfd, wfd):
        super().__init__()
        self.rfile = io.open(rfd, 'rb', 0)
        self.wfile = io.open(wfd, 'wb', 0)
        self.samefds = rfd == wfd

    def close(self):
        self.rfile.close()
        if not self.samefds:
            self.wfile.close()

    def write(self, packet):
        self.wfile.write(packet)

    def read(self, n):
        return self.rfile.read(n)


class PunterStdioTransport(PunterFileTransport):
    def __init__(self):
        super().__init__(sys.stdin.fileno(), sys.stdout.fileno())


class Player:
    def move(self, game):
        pass


class PunterPlayer(Player):
    STATE_SETUP = 1
    STATE_GAMEPLAY = 2
    STATE_SCORING = 3

    def __init__(self):
        super().__init__()
        self.state = self.STATE_SETUP

    def move(self, game):
        self._unpack_state(game)

        self.logger = Logger(self.logfile)
        self.logfile = self.logger.file_name

        self.logger.log('> received')
        self.logger.log(json.dumps(game))

        response = OrderedDict()

        if self.state == self.STATE_SETUP:
            response = self.setup(game)
        elif self.state == self.STATE_GAMEPLAY:
            response = self.gameplay(game)

        if response is not None:
            response = self._pack_state(response)

            self.logger.log('< response')
            self.logger.log(json.dumps(response))
            return response

    def _unpack_state(self, game):
        move = game.get('move', None) or dict()
        self.last_moves = move.get('moves', None) or []

        self.player_state = game.get('state', None) or dict()
        x = self.player_state

        self.logfile = x.get('logfile', None)
        self.state = x.get('state', None) or self.STATE_SETUP
        self.player_id = x.get('player_id', None)
        self.players = x.get('players', 0)
        self.map = x.get('map', None) or dict()

        claims = x.get('claims', list())
        self.claims = set((r[0], r[1]) for r in claims)

    def _pack_state(self, response):
        x = self.player_state
        x['logfile'] = self.logfile
        x['state'] = self.state
        x['player_id'] = self.player_id
        x['players'] = self.players
        x['map'] = self.map
        x['claims'] = list(self.claims)
        response['state'] = x
        return response

    def setup(self, request):
        self.player_id = request.get('punter', None)
        self.map = request.get('map', None)
        self.players = request.get('punters', None)
        self.state = self.STATE_GAMEPLAY
        return self.api_ready()

    def gameplay(self, request):
        if request.get('stop', None) is not None:
            self.state = self.STATE_SCORING
        else:
            return self.make_move()

    def make_move(self):
        self.process_moves()
        river = self.make_claim()
        if river is not None:
            return self.api_claim(river)
        else:
            return self.api_pass()

    def process_moves(self):
        pass

    def make_claim(self):
        pass

    def api_ready(self):
        return {'ready': self.player_id}

    def api_pass(self):
        return {'pass': {'punter': self.player_id}}

    def api_claim(self, river):
        return {'claim': {'punter': self.player_id, 'source': river[0], 'target': river[1]}}


class NoopPlayer(PunterPlayer):
    pass


class RandomPlayer(PunterPlayer):
    def process_moves(self):
        claims = set()
        for move in self.last_moves:
            claim = move.get('claim', None)
            if claim is not None:
                claims.add((claim['source'], claim['target']))
                claims.add((claim['target'], claim['source']))
        self.claims |= claims

    def make_claim(self):
        rivers = self.map['rivers']
        rivers = [(r['source'], r['target']) for r in rivers]
        rivers = set(rivers) - self.claims
        if len(rivers) > 0:
            claim = random.choice(list(rivers))
            return claim


def run_player(player):
    transport = PunterStdioTransport()
    message = transport.receive()
    response = player.move(message)
    transport.send(response)


def play():
    player = RandomPlayer()
    run_player(player)


if __name__ == '__main__':
    play()
