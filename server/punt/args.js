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
env.public_url = process.env.PUBLIC_URL


const parser = new argparse.ArgumentParser({
    description: 'Punter Server (paiv)',
})

parser.add_argument('-b', '--host', '--bind-address', {
    metavar: 'HOST',
    default: defaultAddress,
    help: `Listen address, ${defaultAddress}`,
})

parser.add_argument('-p', '--port', '--bind-port', {
    type: 'int',
    metavar: 'PORT',
    default: undefined,
    help: 'Listen port',
})

parser.add_argument('-w', '--monitor-port', {
    type: 'int',
    metavar: 'MONITOR',
    default: defaultMonitorPort,
    help: `Monitoring port (http), ${defaultMonitorPort}`,
})

parser.add_argument('-m', '--map', {
    default: defaultMap,
    help: `Game map, ${defaultMap}`,
})

parser.add_argument('-n', '--players', {
    type: 'int',
    default: 2,
    help: 'Number of players, 2',
})

parser.add_argument('-f', '--futures', {
    action: 'store_true',
    help: 'Enable futures',
})

parser.add_argument('-o', '--options', {
    action: 'store_true',
    help: 'Enable options',
})

parser.add_argument('-s', '--splurges', {
    action: 'store_true',
    help: 'Enable splurges',
})

parser.add_argument('-th', '--handshake-timeout', {
    type: 'int',
    default: 1,
    help: 'Player timeout on handshake (until "me"), 1 sec',
})

parser.add_argument('-ts', '--setup-timeout', {
    type: 'int',
    default: 10,
    help: 'Player timeout on setup (until "ready"), 10 sec',
})

parser.add_argument('-tm', '--move-timeout', {
    type: 'int',
    default: 1,
    help: 'Player timeout on each move (until response received fully), 1 sec',
})

parser.add_argument('-url', '--public-url', {
    default: undefined,
    help: 'Public URL to access this server',
})



let args = parser.parse_args()


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
