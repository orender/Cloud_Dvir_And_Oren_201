#pragma once
#include <iostream>
#include <map>
#include <stdexcept>
#include <iomanip>
#include <thread>
#include <exception>
#include <iostream>
#include <unordered_map>
#include <sys/stat.h>
#include <mutex>
#include <chrono>
#include "Client.h"
#include "helper.h"
#include "Operations.h"
#include "FileOperation.h"

#pragma comment(lib, "ws2_32.lib")  // Add this lin

#define PORT 12345
#define BUFFER_SIZE 1024

struct Action
{
    int code;
    std::string dataLength;
    std::string data;
    std::string index;
    std::string selectionLength;

    long long timestamp; // Timestamp indicating when the action was created

    std::string fileName;
    std::string fileNameLength;

    std::string msg;
    int size;
    int userId;
};

class Communicator {
private:
    SOCKET m_serverSocket;
    std::atomic<int> clientIdCounter;  // Atomic counter for generating unique client IDs
    std::map<SOCKET, Client*> m_clients; 
    std::map<std::string, Action> m_lastActionMap; // fileName : <lastAction, index>
    std::map<std::string, std::vector<Client>> m_usersOnFile; // fileName : users
    std::vector<std::string> m_files;

    std::unordered_map<std::string, std::mutex> m_fileMutexes;

    Operations operationHandler;
    FileOperation fileOperationHandler;
public:
    // Constructor
    Communicator();

    // Destructor
    ~Communicator();

    void bindAndListen();

    void handleNewClient(SOCKET client_sock);

    void updateFileOnServer(const std::string& filePath, const Action& reqDetail);
    void updateChatFileOnServer(const std::string& filePath, const Action& reqDetail);
    void notifyAllClients(const std::string& updatedContent, SOCKET client_sock);
    void startHandleRequests();

    Action deconstructReq(const std::string& req);
    Action adjustIndexForSync(const std::string& fileName, Action reqDetail);

    void handleClientDisconnect(SOCKET client_sock);

    long long getCurrentTimestamp();

};