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
    // Creating client socket
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        cerr << "Error creating socket!" << endl;
        return 1;
    }

    // Specifying server address
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    // Connecting to server
    if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        cerr << "Error connecting to server!" << endl;
        close(clientSocket);
        return 1;
    }

    // Making socket non-blocking
    fcntl(clientSocket, F_SETFL, O_NONBLOCK);

    cout << "Connected to server. You can start chatting!" << endl;

    // Preparing for select() usage
    fd_set readfds;
    char buffer[1024] = {0};
    while (true) {
        // Clearing the fd_set and add the sockets to it
        FD_ZERO(&readfds);
        FD_SET(clientSocket, &readfds);  // Listen for server socket
        FD_SET(STDIN_FILENO, &readfds);  // Listen for user input (stdin)

        // Using select to check for available input on server socket or stdin
        int max_fd = (clientSocket > STDIN_FILENO) ? clientSocket : STDIN_FILENO;
        int activity = select(max_fd + 1, &readfds, nullptr, nullptr, nullptr);

        if (activity < 0) {
            cerr << "Select error!" << endl;
            break;
        }

        // In case server has sent a message
        if (FD_ISSET(clientSocket, &readfds)) {
            int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
            if (bytesReceived > 0) {
                cout << "Server: " << buffer << endl;

            }
        }

        // In case user wants to send a message to server, enabling further communication
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            string message;
            getline(cin, message);
            
            if (message == "/quit") {
                break;
            }

            send(clientSocket, message.c_str(), message.size(), 0);

            // Saving sent message to chat_history
            ofstream outFile("chat_history", ios::app);
            if (outFile.is_open()) {
                outFile << "Client: " << message << endl;
                outFile.close();
            }
        }
    }

    // Closing the socket
    close(clientSocket);

    return 0;
}


// g++ -o server server.cpp     ./server
// g++ -o client client.cpp     ./client