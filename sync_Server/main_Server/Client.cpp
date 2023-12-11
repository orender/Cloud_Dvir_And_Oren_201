#include "Client.h"

Client::Client(int clientId)
    : id(clientId) 
{
    file_name = "";
}  // Initialize the id field with the provided client ID

Client::~Client()
{
}

int Client::getId() const {
    return id;
}

std::string Client::getFileName() const
{
    return file_name;
}

void Client::setFileName(const std::string newName)
{
    file_name = newName;
}