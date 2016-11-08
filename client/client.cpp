#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "client.h"

constexpr auto myPort = "5000";

void ChatClient::connect_to_server() {

    struct addrinfo hints, *servinfo, *p;
    int connect_fd;
    int yes = 1;

    char ipstr[INET6_ADDRSTRLEN];
    socklen_t addr_size;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // AF_INET or AF_INET6 to force version
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // fill in my IP for me.

    // Change this to be the IP of the server.
    //if ((getaddrinfo("ec2-35-161-207-165.us-west-2.compute.amazonaws.com", myPort, &hints, &servinfo)) != 0) {
    if ((getaddrinfo(nullptr, myPort, &hints, &servinfo)) != 0) {
        std::cout << "Failed to get address info" << std::endl;
        return;
    }

    for (p = servinfo;p != nullptr; p = p->ai_next) {
        connect_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (connect_fd == -1) {
            std::cout << "Failed to get socket file descriptor" << std::endl;
            continue;
        }
         if (connect(connect_fd, p->ai_addr, p->ai_addrlen) == -1) {
             close(connect_fd);
             std::cout << "Failed to connect" << std::endl;
             continue;
         }
        break;
    }

    if (p == nullptr)  {
        std::cout << "Failed to connect to any addresses" << std::endl;
        return;
    }

    freeaddrinfo(servinfo); // done with servinfo.

    std::cout << "client: connecting..." << std::endl;

    set_fd(connect_fd);

    // launch receiving/sending threads
    start();
}

void ChatClient::receive_message() {

    while(true) {

        if (status_ == Status::OFFLINE) return;

        char buf[maxDataSize];

        if (recv(sockfd_, buf, maxDataSize - 1, 0) == -1) {
            std::cout << "Failed to Receive Packet From Server" << std::endl;
            continue;
        }
        // prevent printing out the same message you sent. since server relays message to everyone.
        // TODO: Create a better system, perhaps attach username or IP to message and use that.
        if (strcmp(last_message_sent_.c_str(), buf) == 0) 
            continue;
        std::cout << buf << std::endl;

        // hacky way to shutdown. might not work.
        int buf_size = strlen(buf);
        // 9 is for the string "Shutdown!". Used to compare only the last 9 letters of the message.
        if (strncmp(&buf[buf_size-9], "Shutdown!", 9) == 0) { 
            std::cout << "Server Down. Press a key to exit." << std::endl;
            shutdown();
            return;
        }
    }
}

void ChatClient::send_message() {
    
    char buf[maxDataSize];
    std::string message;
    std::string name;
    std::cout << "Enter desired username" << std::endl;
    std::getline(std::cin, name);
    set_name(name);

    while(true) {

        std::cout << name << ": ";

        std::getline(std::cin, message);

        if (status_ == Status::OFFLINE) return;
        message = name + ": " + message;
        last_message_sent_ = message;

        int mlength = message.length()+1;
        int length = mlength <= maxDataSize ? mlength : maxDataSize;

        strncpy(buf, message.c_str(), length);

        if (send(sockfd_, buf, maxDataSize - 1, 0) == -1) {
            std::cout << "Failed to Send Packet To Server " << std::endl;
        }

        int buf_size = strlen(buf);

        if (strncmp(&buf[buf_size-9], "Shutdown!", 9) == 0) { 
            shutdown();
            return;
        }
    }
}

void ChatClient::shutdown() {

    status_ = Status::OFFLINE;
}
