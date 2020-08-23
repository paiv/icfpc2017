// from stackoverflow: https://stackoverflow.com/questions/901115/how-can-i-get-query-string-values-in-javascript
function getParameterByName(name, url) {
    if (!url) url = window.location.href;
    name = name.replace(/[\[\]]/g, "\\$&");
    var regex = new RegExp("[?&]" + name + "(=([^&#]*)|&|#|$)"),
        results = regex.exec(url);
    if (!results) return null;
    if (!results[2]) return '';
    return decodeURIComponent(results[2].replace(/\+/g, " "));
}


var gameName = getParameterByName("game") || "game";
if (gameName.charAt(gameName.length - 1) == "/") {
// workaround for a bug on kadoban's setup, browsers/server are adding
// a forward slash at the end, screwing up the load
    gameName = gameName.slice(0, -1);
}
var testGame = "../output/" + gameName + ".json";
var TURNS, CURRENT_TURN, SCORES;
var SIGMA_WRAPPER;
var GAME_INFO;
var mines;
var rivers = {};

console.log("GAME NAME",gameName);
$("input#file").val(gameName);

//var colors = ["#00cc00", "#0000cc", "#aaaa00", "#aa00aa", "#00aaaa"];
//var colors = ["#00cc00", "#0000cc", "aaaa00", "aa00aa", "00aaaa"];
//var colors = ["#00ff00", "#0000ff", "aaaa00", "aa00aa", "00aaaa"];
var colors = ["#1f77b4", "#aec7e8", "#aaaa00", "#aa00aa", "#00aaaa"];
//var edgeColor = '#000';
var edgeColor = '#009';//blue uncl river
var mineColor = '#f00';//red mine
var siteColor = '#fff';//white site

function initGame(players) {
    rivers = {};
}

function claim(player, source, target) {
    var river = rivers[source + ";" + target] || rivers[target + ";" + source];
    var edge = SIGMA_WRAPPER.graph.edges(river);
    edge.color = colors[player + 1];
    if (player === null) {
        edge.color = edgeColor;
        edge.size = 2;
    } else {
        edge.color = colors[player];
        edge.size = 10;
    }
    SIGMA_WRAPPER.refresh();
}

function mapToSigma(obj) {
    var res = {"nodes": obj.sites, "edges": obj.rivers};
    mines = new Set(obj.mines);
    for (var n of res.nodes) {
        n.label = String(n.id);
        if (mines.has(n.id)) {
            n.size = 10;
            n.color = mineColor;
        } else {
            n.size = 3;
            n.color = siteColor;
        }
    }
    for (var i in res.edges) {
        var edge = res.edges[i];
        //edge.size = 0.5;
        edge.size = 2;
        edge.color = edgeColor;
        edge.id = i;
        edge.label = edge.source + ";" + edge.target
        rivers[edge.label] = i;
    }
    return res;
}

function loadGraph(path) {
    $.getJSON({
        url: path,
        success: function(response) {
            SIGMA_WRAPPER.graph.clear();
            SIGMA_WRAPPER.graph.read(mapToSigma(response));
            SIGMA_WRAPPER.refresh();
        }
    });
}

function loadGame(path) {
    $("#graph-container").empty();
    SIGMA_WRAPPER = new sigma("graph-container");
    SIGMA_WRAPPER.settings({"minNodeSize": 1, "maxNodeSize": 8,
                    "minEdgeSize": 0.1, "maxEdgeSize": 5,
                    "defaultLabelColor": "#aaaaaa", "labelHoverColor": "#ffffff",
                    "labelThreshold": 0});
    SIGMA_WRAPPER.cameras[0].goTo({ x: 0, y: 0, angle: 0, ratio: 2 });

    $.getJSON({
        url: path,
        success: function(game) {
            var players = [];
            for (var i = 0; i < game.num_players; ++i)
                players[i] = "Punter" + (i + 1);
            initGame(players);
            TURNS = game.turns;
            SCORES = game.scores || {};
            GAME_INFO = game;
            CURRENT_TURN = 0;
            $("#log").text("The game was loaded");

            console.log("GAME IS", game);

            if (game.map) {
              SIGMA_WRAPPER.graph.read(mapToSigma(game.map));
            }

            SIGMA_WRAPPER.refresh();
        }
    });
}

function prevTurn() {
    if (CURRENT_TURN == 0)
        return;
    CURRENT_TURN--;
    drawScoreBoard(CURRENT_TURN-1);
    turn = TURNS[CURRENT_TURN];
    if (turn.claim) {
        var data = turn.claim;
        claim(null, data.source, data.target);
    }

    if (turn.option) {
      var data = turn.option;
      claim(null, data.source, data.target);

    }
    $("#turn" + CURRENT_TURN).remove();
}

function getPunterName(punter) {
  if (GAME_INFO.players) {
    return GAME_INFO.players[punter] || punter;
  }

  return punter;
}

function drawScoreBoard(draw_turn) {
  
  if (typeof(draw_turn) === undefined || !draw_turn) {
    draw_turn = CURRENT_TURN;
  }

  if (SCORES[draw_turn]) {
    $("#scoreboard").empty();
    for (var i in SCORES[draw_turn]) {
      var info = SCORES[draw_turn][i];
      var row =
          $("<span style='margin-left: 10px'>");

      var playerName = $("<span class='player-name' />")
        .text(getPunterName(info.punter));

      var playerSwatch = $("<span class='player-swatch' />");
      playerName.prepend(playerSwatch);
      playerSwatch.css("background-color", colors[i]);

      row
        .append(playerName);

      row
        .append(info.score);


      $("#scoreboard").append(row);
    }
  }

}

function updateGameLog(turn, data) {
      var logEl = $("#log");
      if (turn.claim) {
        logEl.append($("<div>")
          .attr("id", "turn" + CURRENT_TURN)
          .text(getPunterName(data.punter) + " claimed " + data.source + "->" + data.target));
      } else if (turn.pass) {
        logEl.append($("<div>")
          .attr("id", "turn" + CURRENT_TURN)
          .text(getPunterName(turn.pass.punter) + " passed"));

      } else if (turn.option) {
        logEl.append($("<div>")
          .attr("id", "turn" + CURRENT_TURN)
          .text(getPunterName(data.punter) + " optioned " + data.source + "->" + data.target));

      }
}

function nextTurn() {
    if (CURRENT_TURN == TURNS.length) {
        return;
    }

    turn = TURNS[CURRENT_TURN];
    var data;

    if (turn.claim) {
        data = turn.claim;
        claim(data.punter, data.source, data.target);
    } else if (turn.option) {
        data = turn.option;
        claim(data.punter, data.source, data.target);
    }

    drawScoreBoard();
    updateGameLog(turn, data);
    CURRENT_TURN++;
}

$(function() {
    $(':button').on('click', function() {
        loadGame("../output/" + $("#file").val() + ".json");
    });

    $("#back").click(prevTurn);
    $("#forward").click(nextTurn);

    $(document).keydown(function(e) {
        switch (e.keyCode) {
            case 37: prevTurn(); break;
            case 39: nextTurn(); break;
        }
    });

    loadGame(testGame);
});

