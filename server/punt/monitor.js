
const http = require('http')
    , logger = require('./logger')
    , packageVersion = require('../package.json').version


class CacheItem {
    constructor(value, ttl) {
        const now = new Date()
        this.time = now
        this.value = value

        const t = new Date(now)
        t.setMilliseconds(now.getMilliseconds() + ttl)
        this.expires = t
    }

    expired() {
        const now = new Date()
        return this.expires <= now
    }
}


class Monitor {
    constructor(server, match) {
        this.server = server
        this.match = match
    }

    start(host, port) {
        const monitor = this

        this.conn = http.createServer(serve)
        this.conn.on('clientError', clientError)

        this.conn.listen(port, host, () => {
            const addr = this.conn.address()
            logger.debug(`monitor on ${addr.address}:${addr.port}`)
        })


        function clientError(error, socket) {
            socket.end('HTTP/1.1 400 Bad Request\r\n\r\n')
        }

        function serve(request, response) {
            const res = monitor._cachedResponse()

            for (let name in res.headers) {
                response.setHeader(name, res.headers[name])
            }

            response.end(res.body)
        }
    }

    _cachedResponse() {
        if (this.cache && !this.cache.expired()) {
            return this.cache.value
        }

        const doc = this._generateDocument()
        const body = JSON.stringify(doc)
        const now = new Date()
        const item = new CacheItem(null, 1000)

        const response = {
            body: body,
            headers: {
                'Content-Type': 'application/json',
                'Expires': item.expires.toUTCString(),
            }
        }

        item.value = response
        this.cache = item

        return response
    }

    _generateDocument() {
        let doc = {}

        doc.server = this._serverStats()
        doc.map = this._mapInfo()
        doc.match = this._matchStats()
        doc.scoreboard = this._scoreboard()

        return doc
    }

    _serverStats() {
        return {
            uptime: Math.floor(process.uptime()),
            version: packageVersion,
        }
    }

    _mapInfo() {
        return this.match.mapInfo()
    }

    _matchStats() {
        return this.match.stats()
    }

    _scoreboard() {
        return {}
    }
}


module.exports = { Monitor: Monitor }
