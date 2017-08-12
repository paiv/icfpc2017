'use strict'


class Logger {
    constructor() {
        this.DEBUG = process.env.DEBUG
    }

    log() {
        console.log(...arguments)
    }

    error() {
        console.error(...arguments)
    }

    debug() {
        if (this.DEBUG) {
            console.error(...arguments)
        }
    }
}


const logger = new Logger()


module.exports = logger
