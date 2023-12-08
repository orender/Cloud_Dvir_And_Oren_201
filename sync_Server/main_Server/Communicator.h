#pragma once
#include <iostream>
#include <map>
#include <stdexcept>
#include <iomanip>
#include <sstream>
#include <thread>
#include <exception>
#include <iostream>
#include <fstream>
#include <mutex>
#include "Client.h"
#include "helper.h"

#pragma comment(lib, "ws2_32.lib")  // Add this lin

#define PORT 12345
#define BUFFER_SIZE 1024


class Communicator {
private:
    SOCKET m_serverSocket;
    std::atomic<int> clientIdCounter;  // Atomic counter for generating unique client IDs
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

    std::string readFromFile(const std::string& filePath);
    void updateFileOnServer(const std::string& filePath, const std::string& code, const std::string& action);
    void notifyAllClients(const std::string& updatedContent, SOCKET client_sock);
    void startHandleRequests();


    std::pair<std::string, std::string> deconstructReq(const std::string& req);

    void insert(std::fstream& file, const std::string& insertionString);
    void deleteContent(std::fstream& file, const std::string& action);
    void replace(std::fstream& file, const std::string& action);

    void handleClientDisconnect(SOCKET client_sock);

    std::string getFileContent() const {
        return m_fileContent;
    }

};