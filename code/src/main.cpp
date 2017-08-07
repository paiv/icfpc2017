#include <algorithm>
#include <iostream>
#include <fstream>
#include <queue>
#include <random>
#include <string>
#include <unordered_map>
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

template<class T>
inline T
operator& (T a, T b) {
    return static_cast<T>(static_cast<int32_t>(a) & static_cast<int32_t>(b));
}

template<class T>
inline T
operator| (T a, T b) {
    return static_cast<T>(static_cast<int32_t>(a) | static_cast<int32_t>(b));
}

}


namespace paiv {

typedef int32_t s32;
typedef int8_t s8;
typedef uint32_t u32;
typedef uint8_t u8;

typedef vector<s32> ivec;
typedef vector<u32> uvec;
typedef vector<pair<u32,u32>> uuvec;
typedef unordered_set<u32> uset;
typedef unordered_set<pair<u32,u32>> uuset;
typedef vector<uuset> uusetvec;


#if VERBOSE
void log(const char* text) { fprintf(stderr, "%s\n", text); }
void log(const string& text) { log(text.c_str()); }
void log(uint32_t x) { fprintf(stderr, "%u\n", x); }

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

    copy_if(begin(a), end(a), inserter(res, begin(res)),
        [&b] (auto const& x) { return b.find(x) == end(b); }
    );

    return res;
}


template<typename T>
unordered_set<T>
_intersection(const unordered_set<T>& a, const unordered_set<T>& b) {
    unordered_set<T> res;

    copy_if(begin(a), end(a), inserter(res, begin(res)),
        [&b] (auto const& x) { return b.find(x) != end(b); }
    );

    return res;
}


template<typename T>
unordered_set<T>
_union(const unordered_set<T>& a, const unordered_set<T>& b) {
    unordered_set<T> res = a;
    copy(begin(b), end(b), inserter(res, end(res)));
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


template<typename T = u8>
T
_json_parse_bool(const jsonval& obj, const char* name, T default_value = 0) {
    if (obj.HasMember(name)) {
        auto& val = obj[name];
        if (val.IsBool()) {
            return T(val.GetBool());
        }
        if (val.IsNumber()) {
            return T(val.GetInt() != 0);
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
_json_parse_int_int_array(const jsonval& val) {
    Container res;

    for (auto& v : val.GetArray()) {
        if (v.IsArray()) {

            auto vv = _json_parse_int_array<T>(v);

            if (vv.size() >= 2) {
                auto& x = vv[0];
                auto& y = vv[1];

                auto pp = (x < y) ? make_pair(x, y) : make_pair(y, x);

                res.insert(res.end(), pp);
            }
        }
    }

    return res;
}


template<typename T, typename P=pair<T,T>, typename Container=vector<P>>
Container
_json_parse_int_int_array(const jsonval& obj, const char* name) {
    if (obj.HasMember(name)) {
        auto& val = obj[name];
        if (val.IsArray()) {

            return _json_parse_int_int_array<T, P, Container>(val);
        }
    }

    return {};
}


template<typename T, typename P=pair<T,T>>
unordered_set<P>
_json_parse_int_int_array_set(const jsonval& val) {
    return _json_parse_int_int_array<T, P, unordered_set<P>>(val);
}


template<typename T, typename P=pair<T,T>>
unordered_set<P>
_json_parse_int_int_array_set(const jsonval& obj, const char* name) {
    return _json_parse_int_int_array<T, P, unordered_set<P>>(obj, name);
}


template<typename T, typename Alloc>
void
_json_add_int(jsonval& obj, const char* name, T value, Alloc& allocator) {
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
    jsonval val;
    val.SetString(value.c_str(), allocator);
    obj.AddMember(StringRef(name), val, allocator);
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
_json_fill_int_int_array(jsonval& arr, const unordered_set<P<T,T>>& value, Alloc& allocator) {
    for (auto& p : value) {
        jsonval vp(kArrayType);

        vp.PushBack(jsonval().SetInt(get<0>(p)), allocator);
        vp.PushBack(jsonval().SetInt(get<1>(p)), allocator);

        arr.PushBack(vp, allocator);
    }
}


template<typename T, template <typename, typename> class P, typename Alloc>
void
_json_add_int_int_array(jsonval& obj, const char* name, const vector<P<T,T>>& value, Alloc& allocator) {
    jsonval val(kArrayType);

    _json_fill_int_int_array(val, value, allocator);

    obj.AddMember(StringRef(name), val, allocator);
}


template<typename T, template <typename, typename> class P, typename Alloc>
void
_json_add_int_int_array(jsonval& obj, const char* name, const unordered_set<P<T,T>>& value, Alloc& allocator) {
    jsonval val(kArrayType);

    _json_fill_int_int_array(val, value, allocator);

    obj.AddMember(StringRef(name), val, allocator);
}


enum class api_message_type : s32 {
    none = 0,
    // player –> server
    me,
    ready,
    pass,
    claim,
    splurge,
    option,
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


enum class extension : u8 {
    futures = 1,
    splurges = 2,
    options = 4,
};


enum class move_type : u8 {
    pass,
    claim,
    splurge,
    option,
};


typedef struct move_t {
    move_type type;
    s32 player_id;
    pair<u32,u32> claim;
    uvec route;
} move_t;


typedef struct api_state {
    s32 player_id;

    u32 players;
    game_map map;
    u8 ext_futures;
    u8 ext_splurges;
    u8 ext_options;

    u32 options_available;
    uuset options_taken;

    uusetvec claims;
    uuset all_claims;
    ivec score;
} api_state;


typedef struct api {
    api_message_type type;

    s32 player_id;
    api_state state;

    // me
    string player_name;
    // setup
    game_map board;
    u32 players;
    extension settings;
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


extension
_parse_settings(const jsonval& obj, const char* name) {
    extension none = extension(0);

    if (obj.HasMember(name)) {
        auto& val = obj[name];
        if (val.IsObject()) {

            extension fut = _json_parse_bool(val, "futures") ? extension::futures : none;
            extension spl = _json_parse_bool(val, "splurges") ? extension::splurges : none;
            extension opt = _json_parse_bool(val, "options") ? extension::options : none;

            return (fut | spl | opt);
        }
    }

    return none;
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
            else if (v.HasMember("option")) {
                m.type = move_type::option;
                m.player_id = _json_parse_int(v, "option", "punter");
                u32 x = _json_parse_int(v, "option", "source");
                u32 y = _json_parse_int(v, "option", "target");
                m.claim = (x < y) ? make_pair(x, y) : make_pair(y, x);
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


uusetvec
_parse_claims(const jsonval& obj, const char* name, u32 total) {
    uusetvec res(total);

    if (obj.HasMember(name)) {
        auto& val = obj[name];
        if (val.IsArray()) {

            u32 player_id = 0;

            for (auto& p : val.GetArray()) {
                if (player_id < res.size() && p.IsArray()) {
                    auto claims = _json_parse_int_int_array_set<u32>(p);
                    res[player_id] = claims;
                }

                player_id++;
            }
        }
    }

    return res;
}


uuset
_join_claims(const uusetvec& claims) {
    uuset res;

    for (auto& v : claims) {
        copy(begin(v), end(v), inserter(res, res.end()));
    }

    return res;
}


template<typename Alloc>
void
_code_claims(jsonval& obj, const char* name, const uusetvec& claims, Alloc& allocator) {
    jsonval packet(kArrayType);

    for (auto& v : claims) {
        jsonval arr(kArrayType);
        _json_fill_int_int_array(arr, v, allocator);

        packet.PushBack(arr, allocator);
    }

    obj.AddMember(StringRef(name), packet, allocator);
}


api_state
_parse_state(const jsonval& obj, const char* name) {
    api_state state = { .player_id = -1 };

    if (obj.HasMember(name)) {
        auto& val = obj[name];
        if (val.IsObject()) {

            state.player_id = _json_parse_int(val, "me", -1);
            state.players = _json_parse_int(val, "players", 1);
            state.map = _parse_game_map_state(val, "map");
            state.claims = _parse_claims(val, "claims", state.players);
            state.all_claims = _join_claims(state.claims);
            state.score = _json_parse_int_array<s32>(val, "score");

            extension ext = (extension) _json_parse_int(val, "ext", 0);
            state.ext_futures = (ext & extension::futures) == extension::futures;
            state.ext_splurges = (ext & extension::splurges) == extension::splurges;
            state.ext_options = (ext & extension::options) == extension::options;

            state.options_available = _json_parse_int(val, "opts", 0);
            state.options_taken = _json_parse_int_int_array_set<u32>(val, "options");
        }
    }
    return state;
}


template<typename Alloc>
void
_code_state(jsonval& obj, const char* name, const api_state& state, Alloc& allocator) {
    jsonval packet(kObjectType);

    _json_add_int(packet, "me", state.player_id, allocator);
    _json_add_int(packet, "players", state.players, allocator);
    _code_game_map_state(packet, "map", state.map, allocator);
    _code_claims(packet, "claims", state.claims, allocator);
    _json_add_int_array(packet, "score", state.score, allocator);

    const extension none = extension(0);
    extension ext =
        (state.ext_futures ? extension::futures : none) |
        (state.ext_splurges ? extension::splurges : none) |
        (state.ext_options ? extension::options : none);

    _json_add_int(packet, "ext", u32(ext), allocator);

    if (state.ext_options) {
        _json_add_int(packet, "opts", state.options_available, allocator);
        _json_add_int_int_array(packet, "options", state.options_taken, allocator);
    }

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
        message.players = _json_parse_int(packet, "punters", 1);
        message.board = _parse_game_map(packet, "map");
        message.settings = _parse_settings(packet, "settings");
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

        case api_message_type::option:
            _code_claim(packet, "option", message.player_id, message.claim, allocator);
            _code_state(packet, "state", message.state, allocator);
            break;

        case api_message_type::splurge:
            // TODO: splurge
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
    state.players = message.players;
    state.map = message.board;
    state.claims = uusetvec(state.players);
    state.score = ivec(state.players);

    state.ext_futures = (message.settings & extension::futures) == extension::futures;
    state.ext_splurges = (message.settings & extension::splurges) == extension::splurges;
    state.ext_options = (message.settings & extension::options) == extension::options;

    if (state.ext_options) {
        state.options_available = state.map.mines.size();
    }

    response.state = state;

    return response;
}


void
scoring(const api& message) {
    // log("scoring");
}


uuset
_rivers_from_all(const uuset& rivers, const uset& sites) {
    uuset res;
    for (auto &r : rivers) {
        if (sites.find(get<0>(r)) != sites.end() || sites.find(get<1>(r)) != sites.end()) {
            res.insert(r);
        }
    }
    return res;
}


uset
_sites_on_all(const uuset& rivers) {
    uset sites;
    for (auto& r : rivers) {
        sites.insert(get<0>(r));
        sites.insert(get<1>(r));
    }
    return sites;
}


void
_update_claims(const api& message, api_state& state) {
    for (auto& m : message.moves) {
        switch (m.type) {

            case move_type::claim: {
                    state.claims[m.player_id].insert(m.claim);
                    state.all_claims.insert(m.claim);
                }
                break;

            case move_type::option: {
                    state.options_taken.insert(m.claim);
                    if (m.player_id == state.player_id) {
                        state.options_available = max(0, s32(state.options_available) - 1);
                    }
                    state.claims[m.player_id].insert(m.claim);
                    state.all_claims.insert(m.claim);
                }
                break;

            case move_type::splurge: {
                    auto end = m.route.end();
                    auto p = m.route.begin();
                    auto q = p + 1;
                    for (; p != end && q != end; p++, q++) {
                        auto x = *p;
                        auto y = *q;
                        auto claim = (x < y) ? make_pair(x, y) : make_pair(y, x);

                        state.claims[m.player_id].insert(claim);
                        state.all_claims.insert(claim);
                    }
                }
                break;

            case move_type::pass:
                break;
        }
    }
}


void
_calc_shortest_distance(u32* dist, const game_map& board, const uuset& rivers) {
    u32 nsites = board.sites.size();
    memset(dist, 0, sizeof(dist[0]) * nsites * nsites);


    unordered_map<u32, uuset> springs;
    for (auto x : board.sites) {
        springs[x] = {};
    }
    for (auto& r : rivers) {
        springs[get<0>(r)].insert(r);
        springs[get<1>(r)].insert(r);
    }


    using wave_site = pair<u32, u32>;

    struct wave_order {
        bool operator () (const wave_site& a, const wave_site& b) {
            return get<0>(a) > get<0>(b);
        }
    };


    for (auto mine : board.mines) {

        priority_queue<wave_site, uuvec, wave_order> fringe;
        unordered_set<u32> visited;
        fringe.emplace(0, mine);

        while (fringe.size() > 0) {
            auto& it = fringe.top();
            auto wave = get<0>(it);
            auto site = get<1>(it);
            fringe.pop();

            if (visited.find(site) != end(visited)) {
                continue;
            }

            size_t offset = mine * nsites + site;
            auto x = dist[offset];
            auto d = wave * wave;
            dist[offset] = (x > 0) ? min(x, d) : d;

            visited.insert(site);

            for (auto& r : springs[site]) {
                if (get<0>(r) != site)
                    fringe.emplace(wave + 1, get<0>(r));
                if (get<1>(r) != site)
                    fringe.emplace(wave + 1, get<1>(r));
            }
        }
    }
}


ivec
_calc_score(const game_map& board, u32 players, const uusetvec& claims) {
    u32 nsites = board.sites.size();

    static u32* dist = nullptr;
    static u32* player_dist = nullptr;

    if (dist == nullptr) {
        dist = (u32*) malloc(sizeof(u32) * nsites * nsites);
        player_dist = (u32*) malloc(sizeof(u32) * nsites * nsites);

        _calc_shortest_distance(dist, board, board.rivers);
    }


    ivec score(players);

    for (u32 player_id = 0; player_id < players; player_id++) {
        auto& player_claims = claims[player_id];

        _calc_shortest_distance(player_dist, board, player_claims);


        auto sites = _sites_on_all(player_claims);
        u32 player_score = 0;

        for (auto mine : board.mines) {
            for (auto site : sites) {
                size_t offset = mine * nsites + site;
                // if connected mine-site
                if (player_dist[offset] > 0) {
                    // shortest distance squared
                    player_score += dist[offset];
                }
            }
        }

        score[player_id] = player_score;
    }

    return score;
}


void
_update_score(api_state& state) {
    auto score = _calc_score(state.map, state.players, state.claims);
    state.score = score;
}


#if 0
void
random_player(api_state& state, api& response) {
    auto avail = _difference(state.map.rivers, state.all_claims);

    if (avail.size() > 0) {
        auto sel = random_choice(begin(avail), end(avail));

        response.type = api_message_type::claim;
        response.claim = *sel;
    }
}
#endif


#if 0
void
random_mines_player(const api_state& state, api& response) {
    uset visited;
    uset sites(begin(state.map.mines), end(state.map.mines));

    while (sites.size() > 0) {
        auto rivers = _rivers_from_all(state.map.rivers, sites);
        auto avail = _difference(rivers, state.all_claims);

        if (avail.size() > 0) {
            auto sel = random_choice(begin(avail), end(avail));

            response.type = api_message_type::claim;
            response.claim = *sel;
            break;
        }

        copy(begin(sites), end(sites), inserter(visited, visited.begin()));
        auto paths = _intersection(rivers, state.claims[state.player_id]);
        sites = _difference(_sites_on_all(paths), visited);
    }
}
#endif


move_t
_best_move(const uuset& rivers, const api_state& state) {
    move_t res = { move_type::pass };

    if (rivers.size() > 0) {

        s32 best_score = 0;
        uuset avail;

        for (auto& r : rivers) {

            auto claims = state.claims;
            claims[state.player_id].insert(r);

            auto score = _calc_score(state.map, state.players, claims);
            auto new_score = score[state.player_id];

            if (new_score == best_score) {
                avail.insert(r);
            }
            else if (new_score > best_score) {
                best_score = new_score;
                avail = { r };
            }
        }

        if (best_score > state.score[state.player_id]) {
            if (avail.size() > 0) {
                auto sel = random_choice(begin(avail), end(avail));

                res.type = move_type::claim;
                res.claim = *sel;

                if (state.ext_options && state.all_claims.find(res.claim) != end(state.all_claims)) {
                    res.type = move_type::option;
                }
            }
        }
    }

    return res;
}


#if 1
void
best_mines_player(const api_state& state, api& response) {

    uset mines(begin(state.map.mines), end(state.map.mines));
    auto sites = _union(mines, _sites_on_all(state.claims[state.player_id]));
    auto rivers = _rivers_from_all(state.map.rivers, sites);

    uuset avail;
    if (state.ext_options && state.options_available > 0) {
        avail = _difference(rivers, state.options_taken);
    }
    else {
        avail = _difference(rivers, state.all_claims);
    }

    move_t res = { move_type::pass };

    if (avail.size() > 0) {
        res = _best_move(avail, state);
    }

    switch (res.type) {

        case move_type::pass:
            response.type = api_message_type::pass;
            break;

        case move_type::claim:
            response.type = api_message_type::claim;
            response.claim = res.claim;
            break;

        case move_type::option:
            response.type = api_message_type::option;
            response.claim = res.claim;
            break;

        case move_type::splurge:
            break;
    }
}
#endif


api
gameplay(const api& message) {
    api response = { api_message_type::pass };

    response.player_id = message.state.player_id;
    auto state = message.state;

    _update_claims(message, state);
    _update_score(state);


    // random_player(state, response);
    // random_mines_player(state, response);
    best_mines_player(state, response);


    response.state = state;

    return response;
}


#if 0
int
test() {
    api_state state = {};

    game_map sample_map = {};
    sample_map.sites = { 4,1,3,6,5,0,7,2 };
    sample_map.mines = { 1,5 };
    sample_map.rivers = { {5,6},{3,4},{1,7},{1,3},{2,3},{4,5},{5,7},{6,7},{0,7},{3,5},{1,2},{0,1} };

    state.player_id = 1;
    state.players = 2;
    state.map = sample_map;
    state.claims = { { {4, 5} }, {} };
    state.all_claims = { {3, 5}, {4, 5} };
    state.score = {};

    _update_score(state);

    api message = { api_message_type::pass };
    message.state = state;
    auto packet = _code_message(message);

    log(packet);

    return 0;
}
#endif


}


int main(int argc, char* argv[]) {

    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IOLBF, 16*1024);

    // return paiv::test();

    return paiv::player();
}
