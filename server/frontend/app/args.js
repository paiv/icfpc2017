'use strict'

const argparse = require('argparse')
    , url = require('url')


const defaultAddress = '0.0.0.0:8080'
const defaultRefreshDelay = 15000


let env = {}

env.host = process.env.HOST
env.port = parseInt(process.env.PORT)
env.refresh_delay = process.env.REFRESH_DELAY


const parser = new argparse.ArgumentParser({
    description: 'Frontend for Punting Server (paiv/puntd)',
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

parser.addArgument(['-r', '--refresh-delay'], {
    type: 'int',
    defaultValue: defaultRefreshDelay,
    help: `Refresh period, ${defaultRefreshDelay} millis`,
})

parser.addArgument(['node'], {
    nargs: '*',
    help: 'Nodes to monitor',
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

opts.nodes = args.node.map((node) => url.parse(node))


module.exports = opts
