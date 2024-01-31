#include "Client.h"

Client::Client(int clientId, std::string userName, std::string email)
    : _id(clientId), _userName(userName), _email(email)
{
    file_name = "";
}  // Initialize the id field with the provided client ID

Client::Client()
{
}

Client::~Client()
{
}

int Client::getId() const {
    return _id;
}

std::string Client::getFileName() const
{
    return file_name;
}

std::string Client::getUsername() const
{
    return _userName;
}

std::string Client::getPass() const
{
    return _pass;
}

std::string Client::getEmail() const
{
    return _email;
}

void Client::setFileName(const std::string newName)
{
    file_name = newName;
}

void Client::setUsername(const std::string newName)
{
    _userName = newName;
}

void Client::setEmail(const std::string newName)
{
    _email = newName;
}

void Client::setPass(const std::string newName)
{
    _pass = newName;
}

void Client::setId(const int id)
{
    _id = id;
}