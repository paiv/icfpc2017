#include <algorithm>
#include <iostream>
#include <fstream>
#include <random>
#include <string>
#include <unordered_set>
#include <vector>
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"


#define VERBOSE 0
#define PLAYER_NAME "paiv"

using namespace std;
using namespace rapidjson;
using json = rapidjson::Document;
using jsonval = rapidjson::Value;


namespace std {

template<>
struct hash<pair<uint32_t, uint32_t>> {
    inline size_t operator()(const pair<uint32_t, uint32_t> &v) const {
        hash<uint32_t> h;
        size_t seed = 0;
        seed ^= h(v.first) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= h(v.second) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        return seed;
    }
};

}


namespace paiv {

typedef int32_t s32;
typedef int8_t s8;
typedef uint32_t u32;
typedef uint8_t u8;

typedef vector<u32> uvec;
typedef vector<pair<u32,u32>> uuvec;
typedef unordered_set<pair<u32,u32>> uuset;


#if VERBOSE
void log(const char* text) { fprintf(stderr, "%s\n", text); }
void log(const string& text) { log(text.c_str()); }

void log(const json& message) {
    StringBuffer so;
    Writer<StringBuffer> writer(so);
    message.Accept(writer);
    log(so.GetString());
}

#else
#define log(...)
#endif


template<typename T>
unordered_set<T>
_difference(const unordered_set<T>& a, const unordered_set<T>& b) {
    unordered_set<T> res;

    copy_if(a.begin(), a.end(), inserter(res, res.begin()),
        [&b] (auto const& x) { return b.find(x) == b.end(); }
    );

    return res;
}


static random_device _rngeesus;

template<typename Iter>
Iter
random_choice(Iter begin, Iter end) {
    mt19937 gen(_rngeesus());
    uniform_int_distribution<> distr(0, distance(begin, end) - 1);
    advance(begin, distr(gen));
    return begin;
}


void
api_send(const json& message) {
    if (!message.IsNull()) {
        StringBuffer so;
        Writer<StringBuffer> writer(so);
        message.Accept(writer);
        fprintf(stdout, "%lu:%s", so.GetSize(), so.GetString());
    }
}


json
api_receive_json() {
    int n = 0;
    if (fscanf(stdin, "%d:", &n) == 1) {
        if (n > 0) {
            char buffer[n + 1];
            if (fread(buffer, 1, n, stdin) == n) {
                buffer[n] = '\0';

                json message;
                message.Parse(buffer);
                if (message.HasParseError()) {
                    return json(kObjectType);
                }
                return message;
            }
        }
    }
    return json(kObjectType);
}


string
_json_parse_string(const jsonval& obj, const char* name) {
    if (obj.HasMember(name)) {
        auto& val = obj[name];
        if (val.IsString()) {
            return val.GetString();
        }
    }
    return {};
}


template<typename T = s32>
T
_json_parse_int(const jsonval& obj, const char* name, T default_value = 0) {
    if (obj.HasMember(name)) {
        auto& val = obj[name];
        if (val.IsNumber()) {
            return T(val.GetInt());
        }
    }
    return default_value;
}


template<typename T = s32>
T
_json_parse_int(const jsonval& obj, const char* name, const char* inner, T default_value = 0) {
    if (obj.HasMember(name)) {
        auto& val = obj[name];
        if (val.IsObject()) {
            return _json_parse_int(val, inner, default_value);
        }
    }
    return default_value;
}


template<typename T>
vector<T>
_json_parse_int_array(const jsonval& obj) {
    vector<T> value;

    for (auto& v : obj.GetArray()) {
        if (v.IsNumber()) {
            T x = v.GetInt();
            value.push_back(x);
        }
    }

    return value;
}


template<typename T>
vector<T>
_json_parse_int_array(const jsonval& obj, const char* name) {

    if (obj.HasMember(name)) {
        auto& val = obj[name];
        if (val.IsArray()) {

            return _json_parse_int_array<T>(val);
        }
    }

    return {};
}


template<typename T>
vector<T>
_json_parse_int_array(const jsonval& obj, const char* name, const char* inner) {

    if (obj.HasMember(name)) {
        auto& val = obj[name];
        if (val.IsObject()) {

            return _json_parse_int_array<T>(val, inner);
        }
    }

    return {};
}


template<typename T, typename P=pair<T,T>, typename Container=vector<P>>
Container
_json_parse_int_int_array(const jsonval& obj, const char* name) {
    Container value;

    if (obj.HasMember(name)) {
        auto& val = obj[name];
        if (val.IsArray()) {

            for (auto& v : val.GetArray()) {
                if (v.IsArray()) {

                    auto vv = _json_parse_int_array<T>(v);

                    if (vv.size() >= 2) {
                        auto& x = vv[0];
                        auto& y = vv[1];

                        auto pp = (x < y) ? make_pair(x, y) : make_pair(y, x);

                        value.insert(value.end(), pp);
                    }
                }
            }
        }
    }

    return value;
}


template<typename T, typename P=pair<T,T>>
unordered_set<P>
_json_parse_int_int_array_set(const jsonval& obj, const char* name) {
    return _json_parse_int_int_array<T, P, unordered_set<P>>(obj, name);
}


template<typename Alloc>
void
_json_add_int(jsonval& obj, const char* name, s32 value, Alloc& allocator) {
    jsonval val(value);
    obj.AddMember(StringRef(name), val, allocator);
}


template<typename Alloc>
void
_json_add_string(jsonval& obj, const char* name, const char* value, Alloc& allocator) {
    obj.AddMember(StringRef(name), StringRef(value), allocator);
}


template<typename Alloc>
void
_json_add_string(jsonval& obj, const char* name, const string& value, Alloc& allocator) {
    _json_add_string(obj, name, value.c_str(), allocator);
}


template<typename T, typename Alloc>
void
_json_add_int_array(jsonval& obj, const char* name, const vector<T>& value, Alloc& allocator) {
    jsonval val(kArrayType);

    for (auto& x : value) {
        val.PushBack(jsonval().SetInt(x), allocator);
    }

    obj.AddMember(StringRef(name), val, allocator);
}


template<typename T, template <typename, typename> class P, typename Alloc>
void
_json_add_int_int_array(jsonval& obj, const char* name, const vector<P<T,T>>& value, Alloc& allocator) {
    jsonval val(kArrayType);

    for (auto& p : value) {
        jsonval vp(kArrayType);

        vp.PushBack(jsonval().SetInt(get<0>(p)), allocator);
        vp.PushBack(jsonval().SetInt(get<1>(p)), allocator);

        val.PushBack(vp, allocator);
    }

    obj.AddMember(StringRef(name), val, allocator);
}


template<typename T, template <typename, typename> class P, typename Alloc>
void
_json_add_int_int_array(jsonval& obj, const char* name, const unordered_set<P<T,T>>& value, Alloc& allocator) {
    jsonval val(kArrayType);

    for (auto& p : value) {
        jsonval vp(kArrayType);

        vp.PushBack(jsonval().SetInt(get<0>(p)), allocator);
        vp.PushBack(jsonval().SetInt(get<1>(p)), allocator);

        val.PushBack(vp, allocator);
    }

    obj.AddMember(StringRef(name), val, allocator);
}


enum class api_message_type : s32 {
    none = 0,
    // player –> server
    me,
    ready,
    pass,
    claim,
    // server –> player
    you,
    setup,
    move,
    stop,
};


typedef struct game_map {
    uvec sites;
    uvec mines;
    uuset rivers;
} game_map;


enum class move_type : u8 {
    pass,
    claim,
    splurge,
};


typedef struct move_t {
    move_type type;
    s32 player_id;
    pair<u32,u32> claim;
    uvec route;
} move_t;


typedef struct api_state {
    s32 player_id;
    game_map map;
    uuset claims;
} api_state;


typedef struct api {
    api_message_type type;

    s32 player_id;
    api_state state;

    // me
    string player_name;
    // setup
    game_map board;
        // players
        // settings: futures, splurges
    // move
    vector<move_t> moves;
    // claim
    pair<u32,u32> claim;

} api;


vector<u32>
_parse_game_map_sites(const jsonval& obj, const char* name) {
    if (obj.HasMember(name)) {
        auto& val = obj[name];
        if (val.IsArray()) {

            vector<u32> sites;

            for (auto& v : val.GetArray()) {
                auto x = _json_parse_int(v, "id");
                sites.push_back(x);
            }

            return sites;
        }
    }

    return {};
}


uuset
_parse_game_map_rivers(const jsonval& obj, const char* name) {
    if (obj.HasMember(name)) {
        auto& val = obj[name];
        if (val.IsArray()) {

            uuset rivers;

            for (auto& v : val.GetArray()) {
                auto x = _json_parse_int(v, "source");
                auto y = _json_parse_int(v, "target");

                auto pp = (x < y) ? make_pair(x, y) : make_pair(y, x);

                rivers.insert(pp);
            }

            return rivers;
        }
    }

    return {};
}


game_map
_parse_game_map(const jsonval& obj, const char* name) {
    game_map board;

    if (obj.HasMember(name)) {
        auto& val = obj[name];
        if (val.IsObject()) {

            board.sites = _parse_game_map_sites(val, "sites");
            board.mines = _json_parse_int_array<u32>(val, "mines");
            board.rivers = _parse_game_map_rivers(val, "rivers");
        }
    }

    return board;
}


game_map
_parse_game_map_state(const jsonval& obj, const char* name) {
    game_map board;

    if (obj.HasMember(name)) {
        auto& val = obj[name];
        if (val.IsObject()) {

            board.sites = _json_parse_int_array<u32>(val, "sites");
            board.mines = _json_parse_int_array<u32>(val, "mines");
            board.rivers = _json_parse_int_int_array_set<u32>(val, "rivers");
        }
    }

    return board;
}


template<typename Alloc>
void
_code_game_map_state(jsonval& obj, const char* name, const game_map& board, Alloc& allocator) {
    jsonval packet(kObjectType);

    _json_add_int_array(packet, "sites", board.sites, allocator);
    _json_add_int_array(packet, "mines", board.mines, allocator);
    _json_add_int_int_array(packet, "rivers", board.rivers, allocator);

    obj.AddMember(StringRef(name), packet, allocator);
}


vector<move_t>
_parse_moves(const jsonval& obj) {
    vector<move_t> moves;

    for (auto& v : obj.GetArray()) {
        if (v.IsObject()) {
            move_t m = { move_type::pass };

            if (v.HasMember("pass")) {
                m.player_id = _json_parse_int(v, "pass", "punter");
            }
            else if (v.HasMember("claim")) {
                m.type = move_type::claim;
                m.player_id = _json_parse_int(v, "claim", "punter");
                u32 x = _json_parse_int(v, "claim", "source");
                u32 y = _json_parse_int(v, "claim", "target");
                m.claim = (x < y) ? make_pair(x, y) : make_pair(y, x);
            }
            else if (v.HasMember("splurge")) {
                m.type = move_type::splurge;
                m.player_id = _json_parse_int(v, "splurge", "punter");
                m.route = _json_parse_int_array<u32>(v, "splurge", "route");
            }

            moves.push_back(m);
        }
    }

    return moves;
}


vector<move_t>
_parse_moves(const jsonval& obj, const char* name) {
    if (obj.HasMember(name)) {
        auto& val = obj[name];
        if (val.IsObject()) {

            if (val.HasMember("moves")) {
                auto& inner = val["moves"];
                if (inner.IsArray()) {
                    return _parse_moves(inner);
                }
            }
        }
    }
    return {};
}


template<typename Alloc>
void
_code_pass(jsonval& obj, const char* name, s32 player_id, Alloc& allocator) {
    jsonval packet(kObjectType);

    _json_add_int(packet, "punter", player_id, allocator);

    obj.AddMember(StringRef(name), packet, allocator);
}


template<typename Alloc>
void
_code_claim(jsonval& obj, const char* name, s32 player_id, const pair<u32,u32>& river, Alloc& allocator) {
    jsonval packet(kObjectType);

    _json_add_int(packet, "punter", player_id, allocator);
    _json_add_int(packet, "source", get<0>(river), allocator);
    _json_add_int(packet, "target", get<1>(river), allocator);

    obj.AddMember(StringRef(name), packet, allocator);
}


api_state
_parse_state(const jsonval& obj, const char* name) {
    api_state state = { .player_id = -1 };

    if (obj.HasMember(name)) {
        auto& val = obj[name];
        if (val.IsObject()) {

            state.player_id = _json_parse_int(val, "me", -1);
            state.map = _parse_game_map_state(val, "map");
            state.claims = _json_parse_int_int_array_set<u32>(val, "claims");
        }
    }
    return state;
}


template<typename Alloc>
void
_code_state(jsonval& obj, const char* name, const api_state& state, Alloc& allocator) {
    jsonval packet(kObjectType);

    _json_add_int(packet, "me", state.player_id, allocator);
    _code_game_map_state(packet, "map", state.map, allocator);
    _json_add_int_int_array(packet, "claims", state.claims, allocator);

    obj.AddMember(StringRef(name), packet, allocator);
}


api
_parse_message(const json& packet) {
    api message = {};

    if (packet.HasMember("you")) {
        message.type = api_message_type::you;
        message.player_name = _json_parse_string(packet, "you");
    }
    else if (packet.HasMember("map")) {
        message.type = api_message_type::setup;
        message.player_id = _json_parse_int(packet, "punter", -1);
        message.board = _parse_game_map(packet, "map");
        message.state = _parse_state(packet, "state");
    }
    else if (packet.HasMember("move")) {
        message.type = api_message_type::move;
        message.state = _parse_state(packet, "state");
        message.moves = _parse_moves(packet, "move");
    }
    else if (packet.HasMember("stop")) {
        message.type = api_message_type::stop;
        message.state = _parse_state(packet, "state");
    }

    return message;
}


json
_code_message(const api& message) {
    json packet(kObjectType);
    auto& allocator = packet.GetAllocator();

    switch (message.type) {
        case api_message_type::me:
            _json_add_string(packet, "me", message.player_name, allocator);
            break;

        case api_message_type::ready:
            _json_add_int(packet, "ready", message.player_id, allocator);
            _code_state(packet, "state", message.state, allocator);
            break;

        case api_message_type::pass:
            _code_pass(packet, "pass", message.player_id, allocator);
            _code_state(packet, "state", message.state, allocator);
            break;

        case api_message_type::claim:
            _code_claim(packet, "claim", message.player_id, message.claim, allocator);
            _code_state(packet, "state", message.state, allocator);
            break;

        case api_message_type::none:
        case api_message_type::you:
        case api_message_type::setup:
        case api_message_type::move:
        case api_message_type::stop:
            return json(kNullType);
    }

    return packet;
}


api
api_receive() {
    api message = {};

    auto packet = api_receive_json();

    // log("> received");
    log(packet);

    return _parse_message(packet);
}


void
api_send(const api& message) {
    auto packet = _code_message(message);

    if (!packet.IsNull()) {
        // log("< sending");
        log(packet);

        api_send(packet);
    }
}


void
handshake() {
    auto player_name = PLAYER_NAME;

    api message = { api_message_type::me };
    message.player_name = player_name;

    api_send(message);
    api_receive();
}


api setup(const api& message);
api gameplay(const api& message);
void scoring(const api& message);


int
player() {
    handshake();

    auto message = api_receive();

    switch (message.type) {

        case api_message_type::setup:
            api_send(setup(message));
            break;

        case api_message_type::move:
            api_send(gameplay(message));
            break;

        case api_message_type::stop:
            scoring(message);
            break;

        default:
            break;
    }

    return 0;
}


api
setup(const api& message) {
    api response = { api_message_type::ready };
    response.player_id = message.player_id;

    api_state state = {};
    state.player_id = message.player_id;
    state.map = message.board;

    response.state = state;

    return response;
}


api
gameplay(const api& message) {
    api response = { api_message_type::pass };

    response.player_id = message.state.player_id;
    response.state = message.state;

    for (auto& m : message.moves) {
        if (m.type == move_type::claim) {
            response.state.claims.insert(m.claim);
        }
        else if (m.type == move_type::splurge) {
            auto end = m.route.end();
            auto p = m.route.begin();
            auto q = p + 1;
            for (; p != end && q != end; p++, q++) {
                auto x = *p;
                auto y = *q;
                auto claim = (x < y) ? make_pair(x, y) : make_pair(y, x);
                response.state.claims.insert(claim);
            }
        }
    }

    auto avail = _difference(response.state.map.rivers, response.state.claims);

    if (avail.size() > 0) {
        auto sel = random_choice(begin(avail), end(avail));

        response.type = api_message_type::claim;
        response.claim = *sel;
    }

    return response;
}


void
scoring(const api& message) {
    // log("scoring");
}


}

int main(int argc, char* argv[]) {

    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IOLBF, 1000);

    return paiv::player();
}
