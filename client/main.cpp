#include "client.h"

int main() {

    {
        auto cclient = std::make_unique<ChatClient>();

        cclient->connect_to_server();

        while(cclient.get()->get_status() == ChatClient::Status::ONLINE) {
            continue;
        }

        cclient->join_threads();
    } // destory ChatClient object

    return 0;

}
