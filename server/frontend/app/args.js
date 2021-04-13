'use strict'

const argparse = require('argparse')
    , url = require('url')


const defaultAddress = '0.0.0.0:8080'
const defaultRefreshDelay = 15


let env = {}

env.host = process.env.HOST
env.port = parseInt(process.env.PORT)
env.refresh_delay = process.env.REFRESH_DELAY


const parser = new argparse.ArgumentParser({
    description: 'Frontend for Punting Server (paiv/puntd)',
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

parser.add_argument('-r', '--refresh-delay', {
    type: 'int',
    default: defaultRefreshDelay,
    help: `Refresh period, ${defaultRefreshDelay} sec`,
})

parser.add_argument('node', {
    nargs: '*',
    help: 'Nodes to monitor',
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
opts.refresh_delay = opts.refresh_delay * 1000
opts.nodes = args.node.map((node) => url.parse(node))


module.exports = opts
