'use strict'

const logger = require('./logger')
    , net = require('net')


class Client {
    constructor(socket, player) {
        this.socket = socket
        this.player = player
        this.name = `${socket.remoteAddress}:${socket.remotePort}`
        this.data = new Buffer([])

        socket.on('data', (chunk) => this.read(chunk))
        player.on('message', (message) => this.send(message))
    }

    read(chunk) {
        this.data = Buffer.concat([this.data, chunk])

        const offset = this.data.indexOf(':')
        if (offset >= 0) {
            const header = this.data.slice(0, offset)
            const size = parseInt(header)

            if (size) {
                const body = this.data.slice(offset + 1, offset + 1 + size)
                if (body.byteLength >= size) {
                    this.data = this.data.slice(offset + 1 + body.byteLength)

                    try {
                        var message = JSON.parse(body.toString())
                    }
                    catch (e) {
                        logger.error(this.name, e.message)
                        this.socket.destroy()
                        return
                    }

                    this.player.receive(message)
                }
            }
        }
    }

    send(message) {
        const json = JSON.stringify(message)
        const packet = `${json.length}:${json}`
        try {
            this.socket.write(packet)
        }
        catch (e) {
            logger.error(this.name, e.message)
        }
    }
}


class Server {
    constructor(match, players) {
        this.match = match
        this.maxClients = players
        this.clients = []

        this.match.once('close', () => this.stop())
    }

    start(host, port) {
        const server = this
        const match = this.match

        this.conn = net.createServer(onClientConnected)

        // this.conn.maxConnections = this.maxClients + 2 // monitoring
        this.conn.listen(port, host, () => {
            const addr = this.conn.address()
            logger.debug(`listening on ${addr.address}:${addr.port}`)
        })

        function onClientConnected(socket) {
            if (!match.isOpenForNewPlayers()) {
                socket.destroy()
                return
            }

            socket.once('close', () => server.dropClient(socket))

            const player = match.registerNewPlayer()
            if (!player) {
                socket.destroy()
                return
            }

            player.once('close', () => server.dropClient(socket))

            const client = new Client(socket, player)
            server.clients.push(client)
        }
    }

    stop() {
        this.conn.close()
        this.clients.map((cli) => cli.socket)
            .forEach((socket) => socket.end())
    }

    dropClient(socket) {
        const idx = this.clients.findIndex((cli) => cli.socket == socket)
        if (idx >= 0) {
            const client = this.clients.splice(idx, 1)[0]
            client.player.close()
        }
        socket.destroy()
    }
}


module.exports = { Server: Server }
