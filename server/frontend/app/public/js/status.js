function urlparse(value) {
    if (value) {
        if (value.substring(0, 4) !== 'http') {
            value = 'http://' + value;
        }
        var a = document.createElement('a');
        a.href = value;
        return a;
    }
}

function update(obj) {
    var table = document.getElementById('table');
    var body = table.getElementsByTagName('tbody')[0];
    body.innerHTML = '';

    if (!obj) return;

    var nodes = obj.nodes || [];

    for (var i = 0; i < nodes.length; i++) {
        var node = nodes[i] || {};

        if (!node.state || node.state === 'offline') {
            continue;
        }

        var server = node.server || {};
        var map = node.map || {};
        var settings = map.settings || {};
        var match = node.match || {};
        var players = match.players || [];

        var row = body.insertRow(-1)

        var cell1 = row.insertCell(-1)
        var url = urlparse(server.public_url) || {};
        cell1.innerText = url.port || '';

        var cell2 = row.insertCell(-1)
        var spots = [match.spots || 0, match.total_spots || 0].join('/');
        cell2.innerText = spots;

        var cell3 = row.insertCell(-1)
        cell3.innerText = players.map(function(n) { return JSON.stringify(n); }).join(', ');

        var cell4 = row.insertCell(-1)
        cell4.innerText = node.state;

        var cell5 = row.insertCell(-1)
        var textSettings = Object.keys(settings || {}).map(function(k) {
            if (settings[k]) return k;
        }).join(' ');
        cell5.innerHTML = [map.name, textSettings].join('<br>');
    }
}

function onstatus(http) {
    if (http.readyState == 4 && http.status == 200) {
        try {
            var json = JSON.parse(http.responseText);
            update(json);
            return
        }
        catch (e) {
            console.error(e);
        }
    }
    update();
}

function requestUpdate() {
    var http = new XMLHttpRequest();
    http.onreadystatechange = function() { onstatus(http); };
    http.open('GET', '/status', true);
    http.send();
};
