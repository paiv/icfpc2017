#include <iostream>
#include <fstream>
#include <string>
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"


#define VERBOSE 0
#define PLAYER_NAME "paiv"

using namespace std;
using namespace rapidjson;
using json = rapidjson::Document;
using jsonval = rapidjson::Value;

namespace paiv {

typedef int32_t s32;
typedef int8_t s8;
typedef uint32_t u32;
typedef uint8_t u8;


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


enum class game_state : s32 {
    setup = 1,
    gameplay = 2,
    scoring = 3,
};


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


typedef struct api_state {
    s32 player_id;
    game_state state;
} api_state;


typedef struct api {
    api_message_type type;

    string player_name;
    s32 player_id;
    api_state player_state;

} api;


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

s32
_json_parse_int(const jsonval& obj, const char* name, s32 default_value = 0) {
    if (obj.HasMember(name)) {
        auto& val = obj[name];
        if (val.IsNumber()) {
            return val.GetInt();
        }
    }
    return default_value;
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


api_state
_parse_state_object(const jsonval& obj) {
    api_state state = {};

    state.player_id = _json_parse_int(obj, "me", -1);
    state.state = (game_state) _json_parse_int(obj, "state", (s32) game_state::setup);

    return state;
}


api_state
_parse_state(const jsonval& obj, const char* name) {
    if (obj.HasMember(name)) {
        auto& val = obj[name];
        if (val.IsObject()) {
            return _parse_state_object(val);
        }
    }
    return { .player_id = -1 };
}


template<typename Alloc>
void
_code_state_object(const api_state& state, jsonval& obj, Alloc& allocator) {
    jsonval packet(kObjectType);

    _json_add_int(packet, "me", state.player_id, allocator);
    _json_add_int(packet, "state", (s32) state.state, allocator);
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
        message.player_state = _parse_state(packet, "state");
    }
    else if (packet.HasMember("move")) {
        message.type = api_message_type::move;
        message.player_state = _parse_state(packet, "state");
    }
    else if (packet.HasMember("stop")) {
        message.type = api_message_type::stop;
        message.player_state = _parse_state(packet, "state");
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
            break;

        case api_message_type::pass:
            _json_add_int(packet, "pass", message.player_id, allocator);
            break;

        case api_message_type::claim:
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

    log("> received");
    log(packet);

    return _parse_message(packet);
}


void
api_send(const api& message) {
    auto packet = _code_message(message);

    if (!packet.IsNull()) {
        log("< sending");
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

    return response;
}


api
gameplay(const api& message) {
    api response = { api_message_type::pass };

    response.player_id = message.player_id;


    return response;
}


void
scoring(const api& message) {
    log("scoring");
}


}

int main(int argc, char* argv[]) {

    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IOLBF, 1000);

    return paiv::player();
}
