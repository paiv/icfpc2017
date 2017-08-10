'use strict'

const argparse = require('argparse')
    , path = require('path')


const defaultMap = path.join('maps', 'sample.json')


const parser = new argparse.ArgumentParser({
    description: 'Punter Server',
})

parser.addArgument(['-b', '--host', '--bind-address'], {
    metavar: 'HOST',
    defaultValue: 'localhost:9000',
    help: 'Listen address, localhost:9000',
})

parser.addArgument(['-p', '--port', '--bind-port'], {
    type: 'int',
    metavar: 'PORT',
    defaultValue: undefined,
    help: 'Listen port',
})

parser.addArgument(['-m', '--map'], {
    defaultValue: defaultMap,
    help: `Game map (${defaultMap})`,
})

parser.addArgument(['-n', '--players'], {
    type: 'int',
    defaultValue: 2,
    help: 'Number of players, 2',
})

parser.addArgument(['-f', '--futures'], {
    action: 'storeTrue',
    help: 'Enable futures',
})

parser.addArgument(['-o', '--options'], {
    action: 'storeTrue',
    help: 'Enable options',
})

parser.addArgument(['-s', '--splurges'], {
    action: 'storeTrue',
    help: 'Enable splurges',
})

parser.addArgument(['-th', '--handshake-timeout'], {
    type: 'int',
    metavar: 'HANDSHAKE',
    defaultValue: 1,
    help: 'Player timeout on handshake (until "me"), 1 sec',
})

parser.addArgument(['-ts', '--setup-timeout'], {
    type: 'int',
    metavar: 'SETUP',
    defaultValue: 10,
    help: 'Player timeout on setup (until "ready"), 10 sec',
})

parser.addArgument(['-tm', '--move-timeout'], {
    type: 'int',
    metavar: 'MOVE',
    defaultValue: 1,
    help: 'Player timeout on each move (until response received fully), 1 sec',
})


let args = parser.parseArgs()


const address = args.host.split(':'),
    host = address.shift(),
    port = args.port || parseInt(address.shift())

args.host = host
args.port = port

args.timeouts = {
    handshake: args.handshake_timeout * 1000,
    setup: args.setup_timeout * 1000,
    move: args.move_timeout * 1000,
}

args.map = require(path.resolve(args.map))


module.exports = args
