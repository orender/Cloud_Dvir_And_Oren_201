#pragma once
#include "WSAInitializer.h"
#include <string>
#include <iostream>
#include <map>
#include <vector>
#include <thread>
#include <exception>
#include <iostream>
#include <fstream>
#include <mutex>
#include "Client.h"

#pragma comment(lib, "ws2_32.lib")  // Add this lin

#define PORT 12345
#define BUFFER_SIZE 1024

typedef std::vector<char> BUFFER;

class Communicator {
private:
    SOCKET m_serverSocket;
    std::map<SOCKET, Client*> m_clients; // Replace '/*client_handler_type*/' with the actual type
    std::mutex m_fileMutex;
    std::string m_file_name = "test.txt";
    std::string m_fileContent;
public:
    // Constructor
    Communicator();

    // Destructor
    ~Communicator();

    void bindAndListen();

    void handleNewClient(SOCKET client_sock);

    void sendData(const SOCKET sc, const BUFFER message);

    BUFFER getPartFromSocket(const SOCKET sc, const int bytesNum);

    BUFFER getPartFromSocket(const SOCKET sc, const int bytesNum, const int flags);
    std::string readFromFile(const std::string& filePath);
    void updateFileOnServer(const std::string& filePath, const std::string& content);
    void notifyAllClients(const std::string& updatedContent);
    void startHandleRequests();

    std::string getFileContent() const {
        return m_fileContent;
    }

};