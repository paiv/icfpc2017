#include <iostream>
#include <fstream>
#include <string>
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"


#define VERBOSE 1


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
api_receive() {
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


enum class GameState : s32 {
    setup = 1,
    gameplay = 2,
    scoring = 3,
};


void
handshake() {
    auto player_name = "paiv";

    json message(kObjectType);
    jsonval name(player_name, message.GetAllocator());
    message.AddMember("me", name, message.GetAllocator());

    api_send(message);
    api_receive();
}

json setup(const json& message);
json gameplay(const json& message);
json scoring(const json& message);


int
player() {
    log("\nhandshake");
    handshake();

    auto message = api_receive();
    log(message);

    jsonval empty_state(kObjectType);
    jsonval no_value(kNumberType);

    auto& player_state = message.HasMember("state") ? message["state"] : empty_state;
    if (player_state.IsNull()) {
        player_state.SetObject();
    }

    GameState game_state = GameState::setup;

    auto& state = player_state.HasMember("state") ? player_state["state"] : no_value;
    if (state.IsInt()) {
        game_state = (GameState) state.GetInt();
    }

    switch (game_state) {

        default:
        case GameState::setup: {
                auto response = setup(message);
                log(response);
                api_send(response);
            }
            break;

        case GameState::gameplay: {
                auto response = gameplay(message);
                log(response);
                api_send(response);
            }
            break;

        case GameState::scoring:
            scoring(message);
            break;
    }

    return 0;
}


json
setup(const json& message) {
    u32 player_id = message["punter"].GetInt();

    json response(kObjectType);

    jsonval state(kObjectType);
    state.AddMember("state", (u32)GameState::gameplay, response.GetAllocator());
    state.AddMember("me", (u32)player_id, response.GetAllocator());

    response.AddMember("ready", player_id, response.GetAllocator());
    response.AddMember("state", state, response.GetAllocator());

    return response;
}

json gameplay(const json& message) {
    if (message.HasMember("stop")) {
        return scoring(message);
    }

    auto& state = message["state"];
    u32 player_id = state["me"].GetInt();

    json response(kObjectType);
    jsonval player(player_id);
    jsonval mstate(state, response.GetAllocator());

    response.AddMember("pass", player, response.GetAllocator());
    response.AddMember("state", mstate, response.GetAllocator());

    return response;
}


json scoring(const json& message) {
    log("scoring");
    return json();
}


}

int main(int argc, char* argv[]) {

    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IOLBF, 1000);

    return paiv::player();
}
