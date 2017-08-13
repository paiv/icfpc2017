'use strict'

const http = require('http')


class Requests {

    get(options, callback) {
        let data = ''
        const request = http.get(options, (response) => {
            if (response.statusCode !== 200) {
                callback()
                response.resume()
            }
            else {
                response.setEncoding('utf8')
                response.on('data', (chunk) => data += chunk)
                response.on('end', () => {
                    try {
                        const json = JSON.parse(data)
                        callback(json)
                    }
                    catch (e) {
                        callback()
                    }
                })
            }
        })

        request.on('error', (e) => callback())
    }
}


module.exports = new Requests()
