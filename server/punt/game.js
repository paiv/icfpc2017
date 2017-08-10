'use strict'

const logger = require('./logger')
    , random = require('./random')
    , EventEmitter = require('events')
    , timers = require('timers')


const PlayerState = {
    handshake: 0,
    setup: 1,
    gameplay: 2,
    scoring: 3,
    closed: -1,
}


class Player extends EventEmitter {

    constructor(timeouts) {
        super()

        this.name = ''
        this.id = -1

        this.state = PlayerState.handshake
        this.timeouts = timeouts
        this.timeoutsToZombie = 10
        this.zombie = false

        this.handshakeTimeout = timers.setTimeout(() => this._timeout(), this.timeouts.handshake)
    }

    close() {
        this.state = PlayerState.closed

        timers.clearTimeout(this.handshakeTimeout)

        this.emit('close')
    }

    kick() {
        this.close()
    }

    receive(message) {
        // logger.log(JSON.stringify(message))

        if (this.state == PlayerState.handshake) {
            this._handshake(message)
        }
        else if (this.state == PlayerState.setup) {
            this._setup(message)
        }
        else if (this.state == PlayerState.gameplay) {
            this._move(message)
        }
    }

    send(message) {
        // logger.log(JSON.stringify(message))
        if (!this.zombie) {
            this.emit('message', message)
        }
    }

    _timeout() {
        if (this.timeoutsToZombie <= 0) {
            this.zombie = true
        }
        else {
            this.timeoutsToZombie--;
        }

        this.emit('timeout')
    }

    _handshake(message) {
        if ('me' in message) {
            timers.clearTimeout(this.handshakeTimeout)
            this.name = message.me
            this.emit('handshake')
        }
    }

    handshake() {
        this.state = PlayerState.setup
        const response = { you: this.name }
        this.send(response)
    }

    setup(message) {
        this.setupTimeout = timers.setTimeout(() => this._timeout(), this.timeouts.setup)
        this.send(message)
    }

    _setup(message) {
        if ('ready' in message) {
            timers.clearTimeout(this.setupTimeout)
            this.ready(message)
        }
    }

    ready(message) {
        this.state = PlayerState.gameplay
        this.emit('ready', message)
    }

    requestMove(message) {
        if (this.zombie) {
            this._move({pass: {punter: this.id}})
        }
        else {
            this.moveTimeout = timers.setTimeout(() => this._timeout(), this.timeouts.move)
            this.send(message)
        }
    }

    _move(message) {
        timers.clearTimeout(this.moveTimeout)
        this.emit('move', message)
    }

    stopGame(message) {
        this.send(message)
    }
}


const MatchState = {
    handshake: 0,
    setup: 1,
    gameplay: 2,
    scoring: 3,
}


class Match extends EventEmitter {
    constructor(map, players, futures, options, splurges, timeouts) {
        super()

        this.map = map
        this.maxPlayers = players
        this.ext_futures = futures
        this.ext_options = options
        this.ext_splurges = splurges
        this.timeouts = timeouts

        this.players = []
        this.state = MatchState.handshake
    }

    close() {
        this.emit('close')
    }

    isOpenForNewPlayers() {
        return true
    }

    registerNewPlayer() {
        if (this.state != MatchState.handshake) {
            return
        }
        if (this.players.length >= this.maxPlayers) {
            return
        }

        const player = new Player(this.timeouts)
        this.players.push(player)

        player.once('close', () => this.kickPlayer(player))
        player.on('handshake', () => this.handshake(player))
        player.on('ready', (message) => this.ready(player, message))
        player.on('move', (message) => this.commitMove(player, message))
        player.on('timeout', (message) => this.playerTimeout(player))

        return player
    }

    kickPlayer(player) {
        const idx = this.players.indexOf(player)
        this.players.splice(idx, 1)
        player.kick()
    }

    playerTimeout(player) {
        if (this.state == MatchState.handshake) {
            player.send({timeout: this.timeouts.handshake / 1000.0})
            player.close()
        }
        else if (this.state == MatchState.setup) {
            player.send({timeout: this.timeouts.setup / 1000.0})
            this.ready(player, {ready: player.id})
        }
        else if (this.state == MatchState.gameplay) {
            player.send({timeout: this.timeouts.move / 1000.0})
            this.commitMove(player, this._pass(player.id))
        }
    }

    handshake(player) {
        if (this.state == MatchState.handshake) {
            player.handshake()

            const allReady = this.players.length == this.maxPlayers &&
                this.players.every((player) => player.state == PlayerState.setup)

            if (allReady) {
                this.setup()
            }
        }
        else {
            player.kick()
        }
    }

    setup() {
        this.state = MatchState.setup

        this.futures = new Array(this.maxPlayers)

        this.players = random.shuffle(this.players)

        let template = {
            punter: 0,
            punters: this.maxPlayers,
            map: this.map,
        }

        if (this.ext_futures || this.ext_options || this.ext_splurges) {
            template.settings = {}
            if (this.ext_futures) {
                template.settings.futures = true
            }
            if (this.ext_options) {
                template.settings.options = true
            }
            if (this.ext_splurges) {
                template.settings.splurges = true
            }
        }

        this.players.forEach((player, index) => {
            let message = Object.assign({}, template)
            message.punter = index

            player.id = index
            player.setup(message)
        })
    }

    ready(player, message) {
        if (this.state == MatchState.setup) {

            if (this.ext_futures) {
                this.futures[player.id] = this._validate_futures(message.futures)
            }

            const allReady = this.players.every((player) => player.state == PlayerState.gameplay)

            if (allReady) {
                this.startGame()
            }
        }
        else {
            player.kick()
        }
    }

    _validate_futures(futures) {
        const mines = this.map.mines
        const sites = this.map.sites.map((site) => site.id)

        if (Array.isArray(futures)) {
            return futures.filter((f) =>
                mines.indexOf(f.source) >= 0 &&
                    sites.indexOf(f.target) >= 0 &&
                    mines.indexOf(f.target) < 0
            )
        }
        else {
            return []
        }
    }

    startGame() {
        this.state = MatchState.gameplay
        this.turnLimit = this.map.rivers.length
        this.turnCount = 0

        this.rivers = {}
        this.all_claims = {}
        this.all_options = {}
        this.claims = new Array(this.maxPlayers)
        this.options = new Array(this.maxPlayers)
        this.passes = new Array(this.maxPlayers)
        this.moves = []

        this.map.rivers.forEach((river) => this.rivers[this._riverId(river.source, river.target)] = river)

        for (let playerId = 0; playerId < this.maxPlayers; playerId++) {
            this.claims[playerId] = []
            this.options[playerId] = []
            this.moves.push(this._pass(playerId))
        }

        this.activePlayer = 0
        this.requestMove()
    }

    requestMove() {
        const player = this.players[this.activePlayer]

        const moves = {
            moves: this._lastMoves()
        }

        const message = {
            move: moves
        }

        player.requestMove(message)
    }

    _lastMoves() {
        return this.moves.slice(-this.maxPlayers)
    }

    commitMove(player, move) {
        if (this.state == MatchState.gameplay) {
            if (player.id == this.activePlayer) {

                const valid_move = this._validate_move(player.id, move)

                if (valid_move.pass) {
                    const n = this.passes[player.id] || 0
                    this.passes[player.id] = n + 1
                }
                else if (valid_move.claim) {
                    const r = valid_move.claim
                    const link = this._riverId(r.source, r.target)
                    this.all_claims[link] = player.id
                    this.claims[player.id].push(r)
                }
                else if (valid_move.option) {
                    const r = valid_move.option
                    const link = this._riverId(r.source, r.target)
                    this.all_options[link] = player.id
                    this.options[player.id].push(r)
                }
                else if (valid_move.splurge) {
                    const moves = valid_move.splurge.moves
                    delete valid_move.splurge.moves

                    moves.forEach((r) => {
                        if (r.claim) {
                            r = r.claim
                            const link = this._riverId(r.source, r.target)
                            this.all_claims[link] = player.id
                            this.claims[player.id].push(r)
                        }
                        else if (r.option) {
                            r = r.option
                            const link = this._riverId(r.source, r.target)
                            this.all_options[link] = player.id
                            this.options[player.id].push(r)
                        }
                    })
                }

                this.moves.push(valid_move)

                this.activePlayer = (this.activePlayer + 1) % this.maxPlayers
                this.turnCount++;

                if (this.turnCount >= this.turnLimit) {
                    this.stopGame()
                }
                else {
                    this.requestMove()
                }
            }
        }
    }

    _validate_move(playerId, move) {
        const pass = this._pass(playerId)

        if (move.claim) {
            const river = this._riverId(move.claim.source, move.claim.target)

            if (!(river in this.rivers)) {
                return pass
            }

            if (river in this.all_claims) {
                return pass
            }

            return {claim: {punter: playerId, source: move.claim.source, target: move.claim.target}}
        }
        else if (this.ext_options && move.option) {
            const river = this._riverId(move.option.source, move.option.target)

            if (!(river in this.rivers)) {
                return pass
            }

            if (!(river in this.all_claims)) {
                return pass
            }

            if (river in this.all_options) {
                return pass
            }

            if (this.options[playerId].length >= this.map.mines.length) {
                return pass
            }

            return {option: {punter: playerId, source: move.option.source, target: move.option.target}}
        }
        else if (this.ext_splurges && move.splurge) {
            if (!Array.isArray(move.splurge.route)) {
                return pass
            }

            const route = move.splurge.route || []
            const avail = this.passes[playerId] || 0
            let moves = []

            if (route.length < 2) {
                return pass
            }

            if (avail + 1 < route.length - 1) {
                return pass
            }

            let source = route[0]
            let optionsAvail = this.map.mines.length - this.options[playerId].length
            let splurgeClaims = {}
            let splurgeOptions = {}

            for (let i = 1; i < route.length; i++) {
                const target = route[i]
                const river = this._riverId(source, target)
                let isOption = false

                if (!(river in this.rivers)) {
                    return pass
                }

                if (river in splurgeClaims) {
                    return pass
                }

                if (river in this.all_claims) {

                    if (!this.ext_options) {
                        return pass
                    }

                    if (optionsAvail <= 0) {
                        return pass
                    }

                    if (river in this.all_options || river in splurgeOptions) {
                        return pass
                    }

                    isOption = true
                    optionsAvail -= 1
                    splurgeOptions[river] = true
                }
                else {
                    splurgeClaims[river] = true
                }

                const key = isOption ? 'option' : 'claim'
                let mv = {}
                mv[key] = {punter: playerId, source: source, target: target}
                moves.push(mv)

                source = target
            }

            return {splurge: {punter: playerId, route: route, moves: moves}}
        }

        return pass
    }

    _riverId(source, target) {
        if (source > target) {
            target = [source, source = target][0]
        }
        return `${source}-${target}`
    }

    _pass(playerId) {
        return {pass: {punter: playerId}}
    }

    stopGame() {
        this.state = MatchState.scoring

        const scores = this._scores()
        logger.log('scores:', scores)

        let template = {
            scores: scores,
        }

        for (let i = 0; i < this.maxPlayers; i++) {
            let message = {stop: Object.assign({}, template)}

            message.stop.moves = this._lastMoves()

            const player = this.players[this.activePlayer]
            player.stopGame(message)

            this.moves.push(this._pass(this.activePlayer))
            this.activePlayer++;
        }

        this.close()
    }

    _scores() {
        let distance = this._calc_distance(this.map.mines, this.map.sites, this.map.rivers)

        let res = new Array(this.maxPlayers)

        for (let playerId = 0; playerId < this.maxPlayers; playerId++) {
            let score = 0

            let claims = this.claims[playerId]
            if (this.ext_options) {
                claims = claims.concat(this.options[playerId])
            }

            let connected = this._calc_distance(this.map.mines, this.map.sites, claims)

            claims = claims.map((claim) => [claim.source, claim.target])
            claims = [].concat.apply([], claims)
            claims = new Set(claims)

            this.map.mines.forEach((mine) => {
                claims.forEach((site) => {
                    const link = `${mine}-${site}`
                    const d = distance[link] || 0
                    const w = d * d

                    if (connected[link]) {
                        score += w
                    }
                })
            })

            if (this.ext_futures) {
                const futures = this.futures[playerId] || []

                futures.forEach((future) => {
                    const link = `${future.source}-${future.target}`

                    const d = distance[link] || 0
                    const w = d * d * d

                    if (connected[link]) {
                        score += w
                    }
                    else {
                        score -= w
                    }
                })
            }

            res[playerId] = {punter: playerId, score: score}
        }

        return res
    }

    _calc_distance(mines, sites, rivers) {
        const N = sites.length

        let connected = new Array(N)
        for (let i = 0; i < N; i++) {
            connected[i] = new Array(N)
        }

        rivers.forEach((river) => {
            connected[river.source][river.target] = true
            connected[river.target][river.source] = true
        })

        let res = {}

        mines.forEach((mine) => {
            let queue = []
            let visited = new Array(N)
            queue.push([0, mine])

            while (queue.length > 0) {
                const top = queue.shift()
                const wave = top[0]
                    , current = top[1]

                if (visited[current]) {
                    continue
                }

                visited[current] = true

                const link = `${mine}-${current}`
                res[link] = wave

                sites.forEach((site) => {
                    const x = site.id
                    if (connected[current][x]) {
                        queue.push([wave + 1, x])
                    }
                })
            }
        })

        return res
    }
}


module.exports = { Match: Match }
