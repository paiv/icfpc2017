#!/usr/bin/env python3
import io
import json
import socket
import subprocess

from collections import OrderedDict


class PunterError(Exception):
    pass


class PunterTransportError(PunterError):
    pass


class PunterServerError(PunterError):
    pass


class PunterTransport:
    def send(self, obj):
        # print('< sending', obj)
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
        # print('> received', response)
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


class PunterSocketTransport(PunterFileTransport):
    def __init__(self, socket):
        super().__init__(rfd=socket.fileno(), wfd=socket.fileno())
        self.socket = socket

    def close(self):
        super().close()
        self.socket.close()


class PunterMemoryTransport(PunterTransport):
    def __init__(self, initial=None):
        super().__init__()
        self.read_buffer = io.BytesIO(initial)
        self.write_buffer = io.BytesIO()

    def write(self, packet):
        self.write_buffer.write(packet)

    def read(self, n):
        return self.read_buffer.read(n)


class Player:
    def ready(self):
        return False

    def read(self):
        pass

    def write(self, response):
        pass


class OfflinePlayer(Player):
    def __init__(self, cmd, name=None):
        self.cmd = cmd
        self.name = name or 'offline-player'
        self.stdout = None
        self.state = None

    def ready(self):
        return self.state.get('score', None) is None

    def read(self):
        if self.stdout is not None:
            transport = PunterMemoryTransport(self.stdout)
            response = transport.receive()
            self.state = response.pop('state', None)
            return response

    def write(self, response):
        transport = PunterMemoryTransport()

        response = OrderedDict(response)
        response['state'] = self.state

        transport.send(response)
        packet = transport.write_buffer.getvalue()

        res = subprocess.run(self.cmd, input=packet, timeout=1, stdout=subprocess.PIPE, check=True)
        self.stdout = res.stdout


class OnlinePlayer(Player):
    STATE_HANDSHAKE = 0
    STATE_GAMEPLAY = 2
    STATE_SCORING = 3

    def __init__(self, offline_player):
        self.player = offline_player
        self.state = self.STATE_HANDSHAKE

    def ready(self):
        return self.state != self.STATE_SCORING

    def read(self):
        if self.state == self.STATE_HANDSHAKE:
            return self._handshake()
        else:
            return self.player.read()

    def write(self, response):
        if self.state == self.STATE_HANDSHAKE:
            return self._handshake(response)
        else:
            if response.get('stop', None) is not None:
                self.state = self.STATE_SCORING
            return self.player.write(response)

    def _handshake(self, response=None):
        if response is None:
            return self._api_me()
        else:
            self.state = self.STATE_GAMEPLAY

    def _api_me(self):
        return {'me': self.player.name}


class PunterServer:
    HOST = 'punter.inf.ed.ac.uk'
    PORT_BASE = 9000
    MAP1_SAMPLE = 0

    def __init__(self, port=None):
        self.transport = None
        self.port = port
        self.host_ip = socket.gethostbyname(self.HOST)

    def __enter__(self):
        pass

    def __exit__(self, exc_type, exc_value, traceback):
        if self.transport is not None:
            self.transport.close()

    def play(self, mapid, player):
        self._open_map(mapid, port=self.port)

        while player.ready() and self.transport is not None:
            message = player.read()
            while True:
                response = self._send_receive(message)
                message = player.write(response)
                if message is None:
                    break

    def _open_map(self, mapid, port=None):
        if port is not None:
            self.transport = self._connect(port)
        else:
            for n in range(10,0,-1):
                port = self.PORT_BASE + mapid + n
                self.transport = self._connect(port)
                if self.transport is not None:
                    break

        if self.transport is not None:
            print(': connected')
        else:
            raise PunterServerError()

    def _connect(self, port):
        print(': trying', self.host_ip, port)
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(2)
        try:
            sock.connect((self.host_ip, port))
            # sock.settimeout(5)
            sock.setblocking(True)
            return PunterSocketTransport(sock)
        except:
            sock.close()
            return None

    def _send_receive(self, obj=None):
        if obj is not None:
            self.transport.send(obj)

        return self.transport.receive()


def play(mapid, port=None):
    player = OfflinePlayer('./player.py')
    player = OnlinePlayer(player)

    server = PunterServer(port=port)
    with server:
        server.play(mapid, player)


if __name__ == '__main__':
    play(PunterServer.MAP1_SAMPLE, port=None)
