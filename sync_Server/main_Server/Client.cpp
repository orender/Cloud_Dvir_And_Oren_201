#include "Client.h"

Client::Client(int clientId)
    : id(clientId) {}  // Initialize the id field with the provided client ID

Client::~Client()
{
}

int Client::getId() const {
    return id;
}