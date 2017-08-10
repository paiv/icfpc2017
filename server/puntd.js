#!/usr/bin/env node

'use strict'

const logger = require('./punt/logger')
    , args = require('./punt/args')
    , server = require('./punt/server')
    , game = require('./punt/game')


const match = new game.Match(
    args.map,
    args.players,
    args.futures,
    args.options,
    args.splurges,
    args.timeouts)


const srv = new server.Server(match, args.players)

srv.start(args.host, args.port)


// logger.log(`listening on ${args.host}:${args.port}`)
