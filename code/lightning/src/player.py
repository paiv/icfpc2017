#!/usr/bin/env python3 -u
import io
import json
import os
import random
import sys

from datetime import datetime
from collections import OrderedDict


DEBUG=False


class Logger:
    def __init__(self, fn=None, overwrite=False):
        if fn is None:
            now = datetime.now().strftime('%Y%m%d%H%M%S')
            fn = 'logs/%s.log' % now
            if DEBUG:
                os.makedirs('logs', exist_ok=True)

        self.file_name = fn

        if DEBUG:
            mode = 'w' if overwrite else 'a'
            self.file = io.open(fn, mode, buffering=1)

    def log(self, text, *args):
        if DEBUG:
            self.file.write(text)
            for arg in args:
                self.file.write(str(arg))
            self.file.write('\n')


# logger = Logger('offline_player.log', overwrite=True)


class PunterError(Exception):
    pass


class PunterTransportError(PunterError):
    pass


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
            chunk = self.read(1)
            if chunk == b'':
                raise PunterTransportError()

            if chunk is not None:
                chunks.append(chunk)

            data = str(b''.join(chunks), 'utf-8')
            parts = data.split(':', 1)
            if len(parts) == 2:
                body = parts[1]
                total = int(parts[0]) - len(body)
                break

        chunks = []
        body_size = total

        while total > 0:
            chunk = self.read(total)
            if chunk == b'':
                raise PunterTransportError()

            if chunk is not None:
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
    def __init__(self, read_file, write_file):
        super().__init__()
        self.rfile = read_file
        self.wfile = write_file

    def write(self, packet):
        self.wfile.write(packet)

    def read(self, n):
        return self.rfile.read(n)


class PunterFilenoTransport(PunterFileTransport):
    def __init__(self, rfd, wfd):
        self.rfile = io.open(rfd, 'rb', 0)
        self.wfile = io.open(wfd, 'wb', 0)
        self.samefds = rfd == wfd
        super().__init__(self.rfile, self.wfile)

    def close(self):
        self.rfile.close()
        if not self.samefds:
            self.wfile.close()


class PunterStdioTransport(PunterFileTransport):
    def __init__(self):
        super().__init__(sys.stdin.buffer, sys.stdout.buffer)


class PunterPlayer:
    STATE_HANDSHAKE = 0
    STATE_SETUP = 1
    STATE_GAMEPLAY = 2
    STATE_SCORING = 3

    def __init__(self):
        super().__init__()
        self.name = 'paiv'
        self.state = self.STATE_HANDSHAKE

    def handshake(self, response=None):
        if response is None:
            return self._api_me()

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
        self.map['rivers'] = [tuple(sorted((r[0], r[1]))) for r in self.map.get('rivers', list())]

        claims = x.get('claims', list())
        self.claims = set(tuple(sorted((r[0], r[1]))) for r in claims)

        my_claims = x.get('my_claims', list())
        self.my_claims = set(tuple(sorted((r[0], r[1]))) for r in my_claims)

    def _pack_state(self, response):
        response = OrderedDict(response)
        x = self.player_state
        if DEBUG:
            x['logfile'] = self.logfile
        x['state'] = self.state
        x['player_id'] = self.player_id
        x['players'] = self.players
        x['map'] = self.map
        x['claims'] = list(self.claims)
        x['my_claims'] = list(self.my_claims)
        response['state'] = x
        return response

    def setup(self, request):
        self.player_id = request.get('punter', None)
        self.map = request.get('map', None)

        if self.map is not None:
            rivers = self.map.get('rivers', list())
            rivers = [tuple(sorted((r['source'], r['target']))) for r in rivers]
            self.map['rivers'] = rivers

            sites = self.map.pop('sites', list())
            sites = [x['id'] for x in sites]
            self.map['sites'] = sites

        self.players = request.get('punters', None)
        self.state = self.STATE_GAMEPLAY
        self.extra_setup()
        return self._api_ready()

    def extra_setup(self):
        pass

    def gameplay(self, request):
        if request.get('stop', None) is not None:
            self.state = self.STATE_SCORING
        else:
            return self.make_move()

    def make_move(self):
        self.process_moves()
        river = self.make_claim()
        if river is not None:
            return self._api_claim(river)
        else:
            return self._api_pass()

    def process_moves(self):
        pass

    def make_claim(self):
        pass

    def _api_me(self):
        return {'me': self.name}

    def _api_ready(self):
        return {'ready': self.player_id}

    def _api_pass(self):
        return {'pass': {'punter': self.player_id}}

    def _api_claim(self, river):
        return {'claim': {'punter': self.player_id, 'source': river[0], 'target': river[1]}}


class OfflinePlayer:
    def __init__(self, player):
        self.player = player
        self.transport = PunterStdioTransport()

    def run(self):
        message = self.player.handshake()
        self.transport.send(message)

        message = self.transport.receive()
        self.player.handshake(message)

        message = self.transport.receive()
        response = self.player.move(message)
        self.transport.send(response)


class NoopPlayer(PunterPlayer):
    def __init__(self):
        super().__init__()
        if DEBUG:
            self.name = 'paiv-noop'


class RandomPlayer(PunterPlayer):
    def __init__(self):
        super().__init__()
        if DEBUG:
            self.name = 'paiv-random'

    def process_moves(self):
        claims = set()

        for move in self.last_moves:
            claim = move.get('claim', None)
            if claim is not None:
                claims.add(tuple(sorted((claim['source'], claim['target']))))

        self.claims |= claims

    def make_claim(self):
        rivers = self.map['rivers']
        rivers = set(rivers) - self.claims
        if len(rivers) > 0:
            claim = random.choice(list(rivers))
            return claim


class RandomMinesPlayer(RandomPlayer):
    def __init__(self):
        super().__init__()
        if DEBUG:
            self.name = 'paiv-random-mines'

    def process_moves(self):
        claims = set()
        my_claims = set()

        for move in self.last_moves:
            claim = move.get('claim', None)
            if claim is not None:
                r = tuple(sorted((claim['source'], claim['target'])))
                claims.add(r)
                if claim['punter'] == self.player_id:
                    my_claims.add(r)

        self.claims |= claims
        self.my_claims |= my_claims

    def _rivers_from(self, site):
        rivers = self.map['rivers']
        res = set()
        for river in rivers:
            if river[0] == site or river[1] == site:
                res.add(river)
        return res

    def make_claim(self):
        visited = set()
        sites = set(self.map.get('mines', list()))

        while len(sites) > 0:
            rivers = set(r for site in sites for r in self._rivers_from(site))

            avail = rivers - self.claims
            if len(avail) > 0:
                claim = random.choice(list(avail))
                return claim

            visited |= sites
            paths = rivers & self.my_claims
            sites = set(x for p in paths for x in p) - visited


def play():
    player = RandomMinesPlayer()
    controller = OfflinePlayer(player)
    controller.run()


if __name__ == '__main__':
    play()
