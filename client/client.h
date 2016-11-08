/*
    client.h

    A chat client that works by connecting to a hardcoded server on port 5000.
    There are 2 threads that are running.

    1. A sending thread to send messages to the server.
    2. A receiving thread that receive messages from the server.

    Author: Manny Lau


*/
#include <vector>
#include <tuple>
#include <deque>
#include <thread>
#include <string>
#include <unistd.h>

constexpr auto maxDataSize = 100;

class ChatClient {

    public :

        enum class Status {
            ONLINE,
            OFFLINE
        };

        ChatClient() : 
            status_(Status::ONLINE),
            username_("NoOne"),
            last_message_sent_(""),
            sockfd_(-1)
        {}

        // close the socket when destroyed.
        ~ChatClient() {
            close(sockfd_);
        }
        void connect_to_server();
        void receive_message();
        void send_message();
        void shutdown();

        Status get_status() {
            return status_;
        };

        void set_name(std::string & name) {
            username_ = std::move(name);
        }

        void set_fd(const int & fd) {
            sockfd_ = fd;
        }

        void start() {
            client_threads_.emplace_back(std::thread(&ChatClient::receive_message, this));
            client_threads_.emplace_back(std::thread(&ChatClient::send_message, this));
        }

        void join_threads() {

            for(auto & t : client_threads_) {
                t.join();
            }

        }

    private :

        Status status_;
        int sockfd_;
        std::string last_message_sent_;
        std::string username_;
        std::vector<std::tuple<char*,int>> client_fds_;
        std::vector<std::thread> client_threads_;
        std::deque<std::string> messages_queue_;

};
