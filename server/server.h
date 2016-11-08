/*
    server.h

    A chat server that works by accepting TCP connections on port 5000.
    There 3 threads that are running.

    1. An accepting thread that accepts new TCP connections.
    2. A receiving thread which receives messages from clients.
    3. A sending thread that relays received messages to all clients.


    Author: Manny Lau

*/

#include <vector>
#include <tuple>
#include <deque>
#include <thread>
#include <string>
#include <mutex>
#include <iostream>

// Prefetch better.
#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

// Limits messages to 100 characters.
constexpr auto maxDataSize = 100;

// Instantiate class and call start to spawn threads.
class ChatServer {

    public :

        enum class Status {
            ONLINE,
            OFFLINE
        };

        ChatServer() : 
            status_(ChatServer::Status::ONLINE)
        {}

        void accepting();
        void receive_message_from_clients();
        void send_message_to_clients();
        void shutdown();
        Status get_status() {
            return status_;
        };

        void start() {
            client_threads_.emplace_back(std::thread(&ChatServer::accepting, this));
        }

        void join_all_threads() {

            for (auto & t : client_threads_) {
                t.join();
            }

        }

    private :

        Status status_;
        std::mutex c_mutex_;
        std::mutex m_mutex_;
        std::vector<std::tuple<char*,int>> client_fds_;
        std::deque<std::string> messages_queue_;
        std::vector<std::thread> client_threads_;

};
