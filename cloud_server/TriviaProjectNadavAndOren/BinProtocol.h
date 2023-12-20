#pragma once
#include <iostream>
#include <cstdint>
#include <cstring>
#include <WinSock2.h>

#pragma warning(disable:4996) 

// Function to write a message to a buffer
void writeMessage(int code, const std::string& message, char*& buffer, size_t& bufferSize);


// Function to read a message from a buffer
bool readMessage(const char* buffer, int& code, std::string& message);