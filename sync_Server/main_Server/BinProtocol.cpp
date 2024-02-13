#include "BinProtocol.h"

#pragma comment(lib, "ws2_32.lib")

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

// Function to write a message to a buffer
void writeMessage(int code, const std::string& message, char*& buffer, size_t& bufferSize) {
    std::cout << code << "|" << message << std::endl;
    // Create a message
    ProtocolMessage originalMessage;
    originalMessage.code = code;
    strcpy(originalMessage.message, message.c_str());
    bufferSize = MAX_MESSAGE_LENGTH;

    serializeProtocolMessage(originalMessage, buffer);
}

bool readMessage(const char* buffer, int& code, std::string& message) {
    ProtocolMessage recv;
    deserializeProtocolMessage(buffer, recv);
    code = recv.code;
    message = recv.message;
    std::cout << code << "|" << message << std::endl;
    return true;
}