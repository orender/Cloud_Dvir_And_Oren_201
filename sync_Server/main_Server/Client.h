#pragma once
#include <iostream>

class Client
{
public:
    Client(int clientId, std::string userName, std::string email);  // Constructor with ID parameter
    Client();
    ~Client();

    int getId() const;     // Getter for the ID
    std::string getFileName() const;
    std::string getPass() const;
    std::string getEmail() const;
    std::string getUsername() const;

    void setFileName(const std::string newName);
    void setUsername(const std::string newName);
    void setPass(const std::string newName);
    void setEmail(const std::string newName);
    void setId(const int id);

private:
    int _id;                // ID field
    std::string file_name;
    std::string _userName;
    std::string _pass;
    std::string _email;

};
