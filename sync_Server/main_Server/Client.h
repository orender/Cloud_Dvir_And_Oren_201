#pragma once
#include <iostream>

class Client
{
public:
    Client(int clientId);  // Constructor with ID parameter
    ~Client();

    int getId() const;     // Getter for the ID
    std::string getFileName() const;

    void setFileName(const std::string newName);

private:
    int id;                // ID field
    std::string file_name;
};
