'use strict'

const crypto = require('crypto')


class Random {

    shuffle(array) {
        if (array.length > 1) {
            const n = array.length - 1;
            const bytes = [...crypto.randomBytes(n)]
            const random = bytes.map((x,i) => i + Math.floor((n - i) * x / 255.0))
            for (let i = 0; i < n; i++) {
                const j = random[i]
                const temp = array[i]
                array[i] = array[j]
                array[j] = temp
            }
        }
        return array
    }
}


module.exports = new Random()
