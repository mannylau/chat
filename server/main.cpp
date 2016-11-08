#include "server.h"
#include <iostream>

int main() {

    {
        auto cserv = std::make_unique<ChatServer>();

        cserv->start();

        while(likely(ChatServer::Status::ONLINE == cserv.get()->get_status())) {
            continue;
        }

        cserv->join_all_threads();
        
    } // destroy ChatServer

    return 0;

}
