'use strict'

const timers = require('timers')
    , url = require('url')
    , requests = require('./requests')


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

    constructor(nodes) {
        this.nodes = nodes || []

        let status = {}
        this.nodes.forEach((node) => status[node] = {id: node.port})
        this.nodeStatus = status
    }

    start(interval) {
        timers.setInterval(() => this._ontimer(), interval || 15000)
    }

    _ontimer() {
        this.nodes.forEach((node) => {
            requests.get(node, (res) => this._updateNode(node, res))
        })
    }

    _updateNode(node, response) {
        const status = {
            port: node.port,
        }

        Object.assign(status, response || {})

        const match = response && response.match || {}
        status.state = match.state || 'offline'

        this.nodeStatus[node] = status
    }

    status() {
        return this._cachedStatus()
    }

    _cachedStatus() {
        if (this.cache && !this.cache.expired()) {
            return this.cache.value
        }

        const nodes = this.nodes.slice()
        nodes.sort((a, b) => a.port - b.port)

        const status = {
            nodes: nodes.map((node) => Object.assign({}, this.nodeStatus[node]))
        }

        this.cache = new CacheItem(status, 1000)

        return this.cache.value
    }
}


module.exports = { Monitor }
