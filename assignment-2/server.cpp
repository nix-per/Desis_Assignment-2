#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>  // For being able to use the select() function
#include <fstream>  // For being able to access file operations

using namespace std;


int main() {
    // Creating the server socket
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        cerr << "Error creating socket!" << endl;
        return 1;
    }

    // Specifying the server address
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    // Binding the socket to address
    if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        cerr << "Bind failed!" << endl;
        close(serverSocket);
        return 1;
    }

    // Listening for connections
    if (listen(serverSocket, 5) < 0) {
        cerr << "Listen failed!" << endl;
        close(serverSocket);
        return 1;
    }

    cout << "Server started on port 8080..." << endl;

    // Accepting client connection
    int clientSocket = accept(serverSocket, nullptr, nullptr);
    if (clientSocket < 0) {
        cerr << "Error accepting client connection!" << endl;
        close(serverSocket);
        return 1;
    }

    // Making socket non-blocking
    fcntl(clientSocket, F_SETFL, O_NONBLOCK);

    // Preparing for select() usage
    fd_set readfds;
    char buffer[1024] = {0};  // Clear the buffer
    while (true) {
        // Clearing the fd_set and add the sockets to it
        FD_ZERO(&readfds);
        FD_SET(clientSocket, &readfds);  // Listening for client socket
        FD_SET(STDIN_FILENO, &readfds);  // Listening for user input (stdin)

        // Using select to check for available input on client socket or stdin
        int max_fd = (clientSocket > STDIN_FILENO) ? clientSocket : STDIN_FILENO;
        int activity = select(max_fd + 1, &readfds, nullptr, nullptr, nullptr);

        if (activity < 0) {
            cerr << "Select error!" << endl;
            break;
        }

        // In case the client has sent a message
        if (FD_ISSET(clientSocket, &readfds)) {
            int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);  // Prevent buffer overflow
            if (bytesReceived > 0) {
                buffer[bytesReceived] = '\0';  // Null-terminate the received string
                cout << "Client: " << buffer << endl;

                // Clearing buffer after processing
                memset(buffer, 0, sizeof(buffer));
            }
        }

        // In case user wants to send a message to client, enabling further communication
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            string serverMessage;
            getline(cin, serverMessage);
            
            if (serverMessage == "/quit") {
                break;
            }

            // Sending server message to client
            send(clientSocket, serverMessage.c_str(), serverMessage.size(), 0);

            // Saving sent message to chat_history
            ofstream outFile("chat_history", ios::app);
            if (outFile.is_open()) {
                outFile << "Server: " << serverMessage << endl;
                outFile.close();
            }
        }
    }

    // Closing the sockets
    close(clientSocket);
    close(serverSocket);

    return 0;
}


// g++ -o server server.cpp     ./server
// g++ -o client client.cpp     ./client
