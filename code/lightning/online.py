#!/usr/bin/env python3
import json
import select
import socket


class PunterPlayer:
    STATE_HANDSHAKE = 0
    STATE_SETUP = 1
    STATE_GAMEPLAY = 2
    STATE_SCORING = 3

    def __init__(self):
        self.state = self.STATE_HANDSHAKE

    def name(self):
        return 'Player'

    def ready(self):
        return self.state != self.STATE_SCORING

    def read(self):
        # print('** read', self.state)
        if self.state == self.STATE_HANDSHAKE:
            return self.handshake()

    def write(self, response):
        # print('** write', self.state)
        if self.state == self.STATE_HANDSHAKE:
            return self.handshake(response)
        elif self.state == self.STATE_SETUP:
            return self.setup(response)
        elif self.state == self.STATE_GAMEPLAY:
            return self.gameplay(response)

    def handshake(self, response=None):
        if response is None:
            return self.api_me()

        if response.get('you', None) == self.name():
            self.state = self.STATE_SETUP

    def setup(self, request):
        self.player_id = request.get('punter', None)
        self.state = self.STATE_GAMEPLAY
        return self.api_ready()

    def gameplay(self, request):
        if request.get('stop', None) is not None:
            self.state = self.STATE_SCORING
        else:
            return self.api_pass()

    def api_me(self):
        return {'me': self.name()}

    def api_ready(self):
        return {'ready': self.player_id}

    def api_pass(self):
        return {'pass': {'punter': self.player_id}}


class NoopPlayer(PunterPlayer):
    def name(self):
        return 'noop'


class PunterServerError(Exception):
    pass


class PunterServer:
    HOST = 'punter.inf.ed.ac.uk'
    PORT_BASE = 9000
    MAP1_SAMPLE = 0

    def __init__(self, port=None):
        self.socket = None
        self.port = port
        self.host_ip = socket.gethostbyname(self.HOST)

    def __enter__(self):
        pass

    def __exit__(self, exc_type, exc_value, traceback):
        if self.socket is not None:
            self.socket.close()

    def play(self, mapid, player):
        self.player = player
        self._open_map(mapid, port=self.port)

        while self.player.ready() and self.socket is not None:
            message = self.player.read()
            while True:
                response = self._send_receive(message)
                message = self.player.write(response)
                if message is None:
                    break

    def _open_map(self, mapid, port=None):
        if port is not None:
            self.socket = self._connect(port)
        else:
            for n in range(10,0,-1):
                port = self.PORT_BASE + mapid + n
                self.socket = self._connect(port)
                if self.socket is not None:
                    break

        if self.socket is not None:
            print(': connected')
        else:
            raise PunterServerError()

    def _connect(self, port):
        print(': trying', self.host_ip, port)
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(2)
        try:
            sock.connect((self.host_ip, port))
            sock.settimeout(5)
            return sock
        except:
            sock.close()
            return None

    def _send_receive(self, obj=None):
        if obj is not None:
            print('< sending', obj)
            packet = json.dumps(obj, separators=(',', ':'))
            packet = bytes(packet, 'utf-8')
            header = bytes('%d:' % len(packet), 'ascii')
            # print('** ', header, packet)
            self.socket.send(header)
            self.socket.send(packet)

        # s,_,_ = select.select([self.socket], [], [], 10)

        chunks = []

        total = 20
        body = ''

        while True:
            chunk = self.socket.recv(total)
            if chunk == b'':
                raise PunterServerError()

            chunks.append(chunk)

            data = str(b''.join(chunks), 'utf-8')
            parts = data.split(':', 1)
            if len(parts) == 2:
                body = parts[1]
                total = int(parts[0]) - len(body)
                break

        chunks = []

        while total > 0:
            chunk = self.socket.recv(total)
            if chunk == b'':
                raise PunterServerError()

            chunks.append(chunk)
            total -= len(chunk)

        response = b''.join(chunks)
        response = body + str(response, 'utf-8')
        response = json.loads(response)
        print('> received', response)
        return response


def play(mapid, port=None):
    player = NoopPlayer()
    server = PunterServer(port=port)
    with server:
        server.play(mapid, player)


if __name__ == '__main__':
    play(PunterServer.MAP1_SAMPLE, port=None)
