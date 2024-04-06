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
//#include <cpprest/http_client.h>
#include "Client.h"
#include "helper.h"
#include "Operations.h"
#include "FileOperation.h"
#include "IDatabase.h"
#include "BinProtocol.h"

#pragma comment(lib, "ws2_32.lib")  // Add this lin

#define PORT 12345
#define CLOUD_PORT 5555
#define BUFFER_SIZE 4096

struct Action
{
    int code;
    std::string dataLength;
    std::string data;
    std::string index;
    std::string newLineCount;
    std::string selectionLength;

    long long timestamp; // Timestamp indicating when the action was created

    std::string fileName;
    std::string fileNameLength;
    int fileId;

    std::string msg;

    int userNameLength; // login/ signup
    std::string userName;

    std::string pass;
    int passLength;

    std::string email;
    int emailLength;

    int size;
    int userId;
};

class Communicator {
private:
    SOCKET m_serverSocket;
    SOCKET m_cloudServerSocket;
    std::map<SOCKET, Client*> m_clients;
    std::map<std::string, std::vector<Action>> m_lastActionMap; // fileName : <lastAction, index>
    std::map<std::string, std::vector<Client>> m_usersOnFile; // fileName : users

    std::map<std::string, int> m_fileNames; // name, id
    std::map<std::string, std::string> m_filesData; // name, data
    std::map<std::string, bool> m_FileUpdate; // name, there was a change since last update
    
    std::unordered_map<std::string, std::mutex> m_fileMutexes;

    Operations operationHandler;
    FileOperation fileOperationHandler;
    IDatabase* m_database;

    bool CloudConnected;
    bool CloudUpdate;
public:
    // Constructor
    Communicator();

    // Destructor
    ~Communicator();

    void setDB(IDatabase* db);
    void startHandleRequests();
    void bindAndListen();
    void handleNewClient(SOCKET client_sock);

    void cloudCommunicationFunction(/* Parameters for communication */);

    void updateFileOnServerOld(const std::string& filePath, const Action& reqDetail);
    void updateFileOnServer(const std::string& filePath, const Action& reqDetail);

    void handleClientDisconnect(SOCKET client_sock);
    void handleError(SOCKET client_sock, std::exception a);
    long long getCurrentTimestamp();

    void notifyAllClients(const std::string& updatedContent, SOCKET client_sock, const bool isOnFile);
    

    Action deconstructReq(const std::string& req);
    Action adjustIndexForSync(const std::string& fileName, Action reqDetail);

    void login(std::string msg, SOCKET client_sock);
    void signUp(std::string msg, SOCKET client_sock);
    void createFile(std::string msg, SOCKET client_sock);
    void deleteFile(std::string msg, SOCKET client_sock);
    void getFiles(std::string msg, SOCKET client_sock);
    void getInitialContent(std::string msg, SOCKET client_sock);
    void joinFile(std::string msg, SOCKET client_sock);
    void leaveFile(std::string msg, SOCKET client_sock);

};