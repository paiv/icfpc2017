'use strict'


class Logger {
    log() {
        console.log(...arguments)
    }
}


const logger = new Logger()


module.exports = logger
