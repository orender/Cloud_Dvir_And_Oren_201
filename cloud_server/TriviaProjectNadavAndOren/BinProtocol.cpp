#include "BinProtocol.h"

#pragma comment(lib, "ws2_32.lib")

// Function to write a message to a buffer
void writeMessage(int code, const std::string& message, char*& buffer, size_t& bufferSize) {
    uint16_t length = static_cast<uint16_t>(message.length());

    // Calculate total buffer size
    bufferSize = sizeof(int) + sizeof(uint16_t) + length;

    // Allocate buffer
    buffer = new char[bufferSize];

    // Write header
    *reinterpret_cast<int*>(buffer) = htonl(code);
    *reinterpret_cast<uint16_t*>(buffer + sizeof(int)) = length;

    // Write payload
    std::memcpy(buffer + sizeof(int) + sizeof(uint16_t), message.c_str(), length);
}

bool readMessage(const char* buffer, int& code, std::string& message) {
    // Read header
    code = ntohl(*reinterpret_cast<const int*>(buffer));
    buffer += sizeof(int);

    uint16_t length = ntohs(*reinterpret_cast<const uint16_t*>(buffer));
    buffer += sizeof(uint16_t);

    // Read payload
    message.assign(buffer, length);

    return true;
}