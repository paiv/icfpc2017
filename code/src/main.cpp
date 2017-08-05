#include <iostream>
#include <string>
#include "json.hpp"

using namespace std;
using json = nlohmann::json;


namespace paiv {

typedef int32_t s32;
typedef int8_t s8;


void
api_send(const json& message) {
    string packet = message.dump();
    fprintf(stdout, "%lu:%s", packet.size(), packet.c_str());
}


json
api_receive() {
    int n = 0;
    if (fscanf(stdin, "%d:", &n) == 1) {
        if (n > 0) {
            char buffer[n + 1];
            if (fread(buffer, 1, n, stdin) == n) {
                buffer[n] = '\0';

                return json::parse(buffer);
            }
        }
    }
    return json();
}


int
player() {
    json msg;
    msg["me"] = "hello";

    api_send(msg);

    auto res = api_receive();

    return 0;
}

}


int main(int argc, char* argv[]) {

    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IOLBF, 1000);

    return paiv::player();
}
