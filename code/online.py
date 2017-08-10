#!/usr/bin/env python3 -u
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
        chunks = []

        total = 20
        body = ''

        while True:
            chunk = self.read(1)
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


class PunterSocketTransport(PunterFilenoTransport):
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

    def __init__(self, cmd, name=None, logfile=None):
        self.cmd = cmd
        self._name = name or 'offline-player'
        self.state = None
        self.proc = None
        self.transport = None
        logfile = logfile or 'client.log'
        self.clientlog = io.open(logfile, 'a', 1)

    def __enter__(self):
        pass

    def __exit__(self, exc_type, exc_value, traceback):
        self.close()

    def close(self):
        if self.proc is not None:
            self.proc.kill()
            self.proc = None

    @property
    def name(self):
        self._open_process()
        return self._name

    def _open_process(self):
        if self.proc is None:

            self.proc = subprocess.Popen(self.cmd, bufsize=0,
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=self.clientlog)

            self.transport = PunterFileTransport(self.proc.stdout, self.proc.stdin)
            self._handshake()

    def _handshake(self):
        handshake = self.transport.receive()

        if 'me' in handshake:
            self._name = handshake.get('me', None)

            response = {'you': self._name}
            self.transport.send(response)
        else:
            raise PunterError()

    def read(self):
        self._open_process()

        response = self.transport.receive()

        self.state = response.pop('state', None)

        self.close()

        return response

    def write(self, response):
        self._open_process()

        response = OrderedDict(response)
        response['state'] = self.state

        self.transport.send(response)


class OnlinePlayer(Player):
    STATE_HANDSHAKE = 0
    STATE_GAMEPLAY = 2
    STATE_SCORING = 3

    def __init__(self, offline_player, name=None, silent=False):
        self.player = offline_player
        self.name = name
        self.silent = silent
        self.state = self.STATE_HANDSHAKE
        self.handshake_complete = False

    def ready(self):
        return self.state != self.STATE_SCORING

    def read(self):
        if self.state == self.STATE_HANDSHAKE:
            if self.handshake_complete:
                self.state = self.STATE_GAMEPLAY
            else:
                return self._handshake()
        else:
            return self.player.read()

    def write(self, response):
        timeout = response.get('timeout', None)

        if self.state == self.STATE_HANDSHAKE:
            return self._handshake(response)
        elif timeout is not None:
            if not self.silent:
                print('** timeout is ', timeout)
        else:
            if response.get('stop', None) is not None:
                self.state = self.STATE_SCORING
            return self.player.write(response)

    def _handshake(self, response=None):
        if response is None:
            return self._api_me()
        else:
            self.handshake_complete = True

    def _api_me(self):
        return {'me': self.name or self.player.name}


class PunterServer:
    PORT_BASE = 9000
    MAP1_SAMPLE = 0

    def __init__(self, host, port=None, silent=False):
        self.transport = None
        self.host = host
        self.port = port
        self.host_ip = socket.gethostbyname(self.host)
        self.silent = silent

    def __enter__(self):
        pass

    def __exit__(self, exc_type, exc_value, traceback):
        if self.transport is not None:
            self.transport.close()

    def play(self, player):
        self._open_map(port=self.port)

        while player.ready() and self.transport is not None:
            message = player.read()
            while True:
                response = self._send_receive(message)
                timeout = response.get('timeout', None)

                message = player.write(response)

                if timeout is None and message is None:
                    break

    def _open_map(self, port=None):
        if port is not None:
            self.transport = self._connect(port)
        else:
            for n in range(10,0,-1):
                port = self.PORT_BASE + n
                self.transport = self._connect(port)
                if self.transport is not None:
                    break

        if self.transport is not None:
            if not self.silent:
                print(': connected')
        else:
            raise PunterServerError('could not connect')

    def _connect(self, port):
        if not self.silent:
            print(': trying', self.host, port)

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


def play(name, host, port, cmd, logfile, silent=False):
    player = OfflinePlayer(cmd, logfile=logfile)
    player = OnlinePlayer(player, name=name, silent=silent)

    server = PunterServer(host=host, port=port, silent=silent)
    with server:
        server.play(player)


if __name__ == '__main__':
    import argparse
    import os

    # default_host = 'punter.inf.ed.ac.uk'
    default_host = '127.0.0.1'

    default_cmd = os.path.join(os.path.dirname(__file__), 'punter')


    parser = argparse.ArgumentParser(description='Online to offline runner (lamduct)')

    parser.add_argument('-a', '--host', type=str,
        default=default_host,
        help='server host, %s' % default_host)

    parser.add_argument('-p', '--port', type=int,
        help='server port')

    parser.add_argument('-n', '--name', type=str, default='online-player',
        help='player name')

    parser.add_argument('--cmd', type=str, default=default_cmd,
        help='offline client command line')

    parser.add_argument('--log', type=str, default='client.log',
        help='client log file')

    parser.add_argument('-s', '--silent', action='store_true',
        help='be quiet')

    args = parser.parse_args()


    play(
        name=args.name,
        host=args.host,
        port=args.port,
        cmd=args.cmd,
        logfile=args.log,
        silent=args.silent)
