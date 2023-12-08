#pragma once
class Client
{
public:
    Client(int clientId);  // Constructor with ID parameter
    ~Client();

    int getId() const;     // Getter for the ID

private:
    int id;                // ID field
};
