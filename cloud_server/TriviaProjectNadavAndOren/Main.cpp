#include "Winsock2.h"
#include "container.h"
#include "dataSplitter.h"
#include <iostream>
#include "DBHelper.hpp"
#include <cstring>
#include <cstdint>
#define PORT 5555
#define GOODCOMMAND 420
#define BADCOMMAND 42

// Define the maximum length of the message
const int MAX_MESSAGE_LENGTH = 1024;

// Define the structure of the protocol
struct ProtocolMessage {
    int code;
    char message[MAX_MESSAGE_LENGTH];
};

// Function to serialize the ProtocolMessage into a binary buffer
void serializeProtocolMessage(const ProtocolMessage& message, char* buffer) {
    // Copy the code into the buffer (4 bytes for int)
    memcpy(buffer, &message.code, sizeof(int));

    // Copy the message into the buffer after the code
    memcpy(buffer + sizeof(int), message.message, strlen(message.message) + 1);
}

// Function to deserialize the binary buffer into a ProtocolMessage
void deserializeProtocolMessage(const char* buffer, ProtocolMessage& message) {
    // Copy the code from the buffer (4 bytes for int)
    memcpy(&message.code, buffer, sizeof(int));

    // Copy the message from the buffer after the code
    memcpy(message.message, buffer + sizeof(int), MAX_MESSAGE_LENGTH);
}

int main() 
{

    WSADATA              wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    
    dataSplitter ds = dataSplitter("test.db");
    

    SOCKET listner = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    SOCKET m_syncServerSocket;
    struct sockaddr_in sa = { 0 };

    sa.sin_port = htons(PORT); // port that server will listen for
    sa.sin_family = AF_INET;   // must be AF_INET
    sa.sin_addr.s_addr = INADDR_ANY;    // when there are few ip's for the machine. We will use always "INADDR_ANY"

    // Connects between the socket and the configuration (port and etc..)
    if (bind(listner, (struct sockaddr*)&sa, sizeof(sa)) == SOCKET_ERROR)
        throw std::exception("Failed to bind onto the requested port");

    // Start listening for incoming requests of clients
    if (listen(listner, SOMAXCONN) == SOCKET_ERROR)
        throw std::exception("Failed listening to requests.");

    m_syncServerSocket = accept(listner, NULL, NULL);
    if (m_syncServerSocket == INVALID_SOCKET)
        throw std::exception("Recieved an invalid socket.");

    std::string nameLen = "";
    std::string name = "";
    std::string data = "";
    std::string a = "";
    char* buffer;
    size_t bufferSize = MAX_MESSAGE_LENGTH;
    ProtocolMessage originalMessage;
    ProtocolMessage receivedMessage;

    while (true)
    {
        char recvbuf[1024];
        recv(m_syncServerSocket, recvbuf, 1024, 0);
        int code = 0;
        std::string msg = "";
        deserializeProtocolMessage(recvbuf, receivedMessage);
        code = receivedMessage.code;
        msg = receivedMessage.message;

        std::cout << code << "|" << msg << std::endl;

        switch (code)
        {
        case (int)saveBlobCode:
            nameLen = msg.substr(0, 3);
            name = msg.substr(3, std::stoi(nameLen));
            data = msg.substr(std::stoi(nameLen)+3);

            std::cout << "file name: " << name << ", file data:\n" << msg << std::endl;

            if (ds.saveNewFile(name, data))
            {

                buffer = new char[1024];
                a = "file saved successfully";
                originalMessage.code = GOODCOMMAND;
                strcpy(originalMessage.message, a.c_str());
                serializeProtocolMessage(originalMessage, buffer);

                send(m_syncServerSocket, buffer, bufferSize, 0);
            }
            else {

                buffer = new char[1024];
                a = "file was not saved successfully";
                originalMessage.code = BADCOMMAND;
                strcpy(originalMessage.message, a.c_str());
                serializeProtocolMessage(originalMessage, buffer);

                send(m_syncServerSocket, buffer, bufferSize, 0);
            }
            break;
        case (int)getBlobCode:

            buffer = new char[1024];
            a = ds.getFileData(msg);
            originalMessage.code = GOODCOMMAND;
            strcpy(originalMessage.message, a.c_str());
            serializeProtocolMessage(originalMessage, buffer);

            send(m_syncServerSocket, buffer, bufferSize, 0);
            break;
        case (int)deleteBlobCode:
            if (ds.deleteFile(msg))
            {
                buffer = new char[1024];
                a = "file was deleted successfully";
                originalMessage.code = GOODCOMMAND;
                strcpy(originalMessage.message, a.c_str());
                serializeProtocolMessage(originalMessage, buffer);

                send(m_syncServerSocket, buffer, bufferSize, 0);
            }
            else {
                buffer = new char[1024];
                a = "file was not deleted successfully";
                originalMessage.code = BADCOMMAND;
                strcpy(originalMessage.message, a.c_str());
                serializeProtocolMessage(originalMessage, buffer);

                send(m_syncServerSocket, buffer, bufferSize, 0);
            }
            break;
        case (int)getAllFiles:
            buffer = new char[1024];
            a = ds.getFiles();
            originalMessage.code = GOODCOMMAND;
            strcpy(originalMessage.message, a.c_str());
            serializeProtocolMessage(originalMessage, buffer);

            send(m_syncServerSocket, buffer, bufferSize, 0);
            break;
        case (int)createNewFile:
            if (ds.saveNewFile(msg, ""))
            {

                buffer = new char[1024];
                a = "file created successfully";
                originalMessage.code = GOODCOMMAND;
                strcpy(originalMessage.message, a.c_str());
                serializeProtocolMessage(originalMessage, buffer);

                send(m_syncServerSocket, buffer, bufferSize, 0);
            }
            else {

                buffer = new char[1024];
                a = "file was not created successfully";
                originalMessage.code = BADCOMMAND;
                strcpy(originalMessage.message, a.c_str());
                serializeProtocolMessage(originalMessage, buffer);

                send(m_syncServerSocket, buffer, bufferSize, 0);
            }
            break;
        default:
            break;
        }

    }
    WSACleanup();

    return 0;
}