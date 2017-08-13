#!/usr/bin/env node

'use strict'

const args = require('./app/args')
    , mon = require('./app/monitor')
    , express = require('express')
    , cache = require('apicache')
    , app = express()


app.use(cache.middleware('1 second'))
app.use(express.static('./app/public'))
app.set('view engine', 'pug')
app.set('views', 'app')


const monitor = new mon.Monitor(args.nodes)
monitor.start(args.refresh_delay)


app.get('/', (request, response) => {
    response.render('index')
})

app.get('/status', (request, response) => {
    response.json(monitor.status())
})


app.listen(args.port, args.host)
