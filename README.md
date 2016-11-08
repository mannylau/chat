To compile the chat client : g++ -std=c++14 main.cpp client.cpp -o client
To compile the chat server : g++ -std=c++14 main.cpp server.cpp -o server

You may need to include the -lpthread flag when compiling.

You are able to run the server on your own machine.
You can also run multiple clients to chat with yourself.

Known Issues that will be addressed:

    Shutdown Mechanism
    Receiving Duplicate Messages if more than 100 characters are typed
    Improper Shutdown causes problems to connected clients
