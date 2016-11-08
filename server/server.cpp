#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <chrono>
#include <cstring>
#include "server.h"

constexpr auto myPort = "5000";
constexpr auto listenQueueMax = 10;


void ChatServer::accepting() {

    struct sockaddr_storage their_addr;
    struct addrinfo hints, *servinfo, *p;
    // listen on sock_fd, new connection on new_fd
    int accepting_fd, new_fd; 
    int yes = 1;

    char ipstr[INET6_ADDRSTRLEN];
    socklen_t addr_size;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // AF_INET or AF_INET6 to force version
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // fill in my IP for me.

    if (getaddrinfo(nullptr, myPort, &hints, &servinfo) != 0) {
        std::cout << "Failed to get address info" << std::endl;
        return;
    }

    // Loop through possible fds to obtain a usable fd.
    for (p = servinfo;p != nullptr; p = p->ai_next) {
        accepting_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (accepting_fd == -1) {
            std::cout << "Failed to get socket file descriptor" << std::endl;
            continue;
        }

        // Reuse address if already in use.
        if (setsockopt(accepting_fd, SOL_SOCKET, SO_REUSEADDR, &yes,sizeof yes) == -1) {
            std::cout << "Failed to set socket options" << std::endl;
            return;
        } 

        // Make the accept call non blocking, so other threads can run.
        fcntl(accepting_fd, F_SETFL, O_NONBLOCK);

        // Bind the socket to a port.
        if (bind(accepting_fd, p->ai_addr, p->ai_addrlen) == -1) {
            close(accepting_fd);
            std::cout << "Failed to bind" << std::endl;
            continue;
        }
        break;
    }

    if (p == nullptr)  {
        std::cout << "Failed to bind to any addresses" << std::endl;
        return;
    }

    // print out own server ip.
    inet_ntop(p->ai_family, &p->ai_addr, ipstr, sizeof ipstr);
    std::cout << "My IP is : " << ipstr << std::endl;

    // good practice to free address info.
    freeaddrinfo(servinfo);

    // Start listening for connections.
    if (listen(accepting_fd, listenQueueMax) == -1) {
        std::cout << "Failed to listen" << std::endl;
        return;
    }

    std::cout << "server: waiting for connections..." << std::endl;

    // launch the receive/send threads
    client_threads_.emplace_back(std::thread(&ChatServer::receive_message_from_clients, this));
    client_threads_.emplace_back(std::thread(&ChatServer::send_message_to_clients, this));

    while(true) {

        if (unlikely(status_ == Status::OFFLINE)) return;

        addr_size = sizeof(their_addr);
        new_fd = accept(accepting_fd, (struct sockaddr*)&their_addr, &addr_size);

        // Since accept call is non blocking, accepting a new connection will be rare.
        if (likely(new_fd == -1)) {
            continue;
        }

        // get client's ipstr. only ipv4right now.
        inet_ntop(their_addr.ss_family,
                &(((struct sockaddr_in*)&their_addr)->sin_addr),
                ipstr, sizeof ipstr);

        std::cout << "server: got connection from: "<< ipstr << std::endl;

        auto tup = std::make_tuple(ipstr, new_fd);

        {
            std::lock_guard<std::mutex> lg(m_mutex_);
            client_fds_.push_back(tup);
        }

        if (send(new_fd, "Connected to Manny's Server", 27, 0) == -1) {
            std::cout << "Error sending welcome message to client" << std::endl;
        }
    }
}

void ChatServer::receive_message_from_clients() {

    while(true) {

        if (unlikely(status_ == Status::OFFLINE)) return;

        {
            std::lock_guard<std::mutex> lg(m_mutex_);

            for (auto &tup : client_fds_) {

                char buf[maxDataSize];

                if (recv(std::get<1>(tup), buf, maxDataSize - 1, MSG_DONTWAIT) == -1) {
                    continue;
                }

                messages_queue_.push_back(std::string(buf));
                std::cout << "Got a message from: " << std::get<0>(tup) << std::endl;

            }
        }
    }
}

void ChatServer::send_message_to_clients() {

    while(true) {

        {
            std::lock_guard<std::mutex> lg(m_mutex_);

            if (!messages_queue_.empty()) {

                char buf[maxDataSize];
                strncpy(buf, messages_queue_.front().c_str(), maxDataSize);
                int buf_size = strlen(buf);

                for (auto &tup : client_fds_) {

                    if (send(std::get<1>(tup), buf, maxDataSize - 1, MSG_DONTWAIT) == -1) {
                        std::cout << "Failed to Send Packet To: " << std::get<0>(tup) << std::endl;
                    }
                }

                // display chat message received.a
                // std::cout << messages_queue_.front() << std::endl;

                // Current shutdown mechanism gives client's the ability to shutdown. By typing Shutdown!.
                if (strncmp(&buf[buf_size-9], "Shutdown!", 9) == 0) { 
                    shutdown();
                }

                messages_queue_.pop_front();
            }
        }

        if (unlikely(Status::OFFLINE == status_)) return;

    }
}


void ChatServer::shutdown() {

    for (auto tuple : client_fds_) {
        close(std::get<1>(tuple));
    }

    status_ = Status::OFFLINE;
}
