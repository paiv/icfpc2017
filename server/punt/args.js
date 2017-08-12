'use strict'

const argparse = require('argparse')
    , path = require('path')


const defaultMap = path.join('maps', 'sample.json')
const defaultAddress = '0.0.0.0:9000'
const defaultMonitorPort = 8080


let env = {}

env.host = process.env.HOST
env.port = parseInt(process.env.PORT)
env.monitor_port = parseInt(process.env.MONITOR_PORT)
env.map = process.env.MAP
env.players = parseInt(process.env.PLAYERS)
env.futures = process.env.FUTURES
env.options = process.env.OPTIONS
env.splurges = process.env.SPLURGES
env.handshake_timeout = parseInt(process.env.HANDSHAKE_TIMEOUT)
env.setup_timeout = parseInt(process.env.SETUP_TIMEOUT)
env.move_timeout = parseInt(process.env.MOVE_TIMEOUT)


const parser = new argparse.ArgumentParser({
    description: 'Punter Server (paiv)',
})

parser.addArgument(['-b', '--host', '--bind-address'], {
    metavar: 'HOST',
    defaultValue: defaultAddress,
    help: `Listen address, ${defaultAddress}`,
})

parser.addArgument(['-p', '--port', '--bind-port'], {
    type: 'int',
    metavar: 'PORT',
    defaultValue: undefined,
    help: 'Listen port',
})

parser.addArgument(['-w', '--monitor-port'], {
    type: 'int',
    metavar: 'MONITOR',
    defaultValue: defaultMonitorPort,
    help: `Monitoring port (http), ${defaultMonitorPort}`,
})

parser.addArgument(['-m', '--map'], {
    defaultValue: defaultMap,
    help: `Game map, ${defaultMap}`,
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
    defaultValue: 1,
    help: 'Player timeout on handshake (until "me"), 1 sec',
})

parser.addArgument(['-ts', '--setup-timeout'], {
    type: 'int',
    defaultValue: 10,
    help: 'Player timeout on setup (until "ready"), 10 sec',
})

parser.addArgument(['-tm', '--move-timeout'], {
    type: 'int',
    defaultValue: 1,
    help: 'Player timeout on each move (until response received fully), 1 sec',
})


let args = parser.parseArgs()


let opts = {}

for (let key in env) {
    opts[key] = env[key] || args[key]
}


const address = opts.host.split(':'),
    host = address.shift(),
    port = opts.port || parseInt(address.shift())

opts.host = host
opts.port = port

opts.timeouts = {
    handshake: opts.handshake_timeout * 1000,
    setup: opts.setup_timeout * 1000,
    move: opts.move_timeout * 1000,
}


const filename = path.resolve(opts.map)
opts.map = require(filename)
opts.map.name = path.basename(filename)


module.exports = opts
