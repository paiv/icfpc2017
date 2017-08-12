#!/usr/bin/env node

'use strict'

const logger = require('./punt/logger')
    , args = require('./punt/args')
    , server = require('./punt/server')
    , game = require('./punt/game')
    , monitor = require('./punt/monitor')


const match = new game.Match(
    args.map,
    args.players,
    args.futures,
    args.options,
    args.splurges,
    args.timeouts)


const srv = new server.Server(match, args.players)


if (Number.isInteger(args.monitor_port)) {
    const mon = new monitor.Monitor(srv, match)
    mon.start(args.host, args.monitor_port)
}


srv.start(args.host, args.port)
