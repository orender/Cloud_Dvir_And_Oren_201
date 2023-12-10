#include "Communicator.h"

Communicator::Communicator()
{
    // this server use TCP. that why SOCK_STREAM & IPPROTO_TCP
    // if the server use UDP we will use: SOCK_DGRAM & IPPROTO_UDP
    m_serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_serverSocket == INVALID_SOCKET)
        throw std::exception("Failed to initialize server socket.");
}

// Destructor
Communicator::~Communicator() {
    try {
        closesocket(m_serverSocket);
    }
    catch (...) {}
}

void Communicator::bindAndListen()
{
    struct sockaddr_in sa = { 0 };

    sa.sin_port = htons(PORT); // port that server will listen for
    sa.sin_family = AF_INET;   // must be AF_INET
    sa.sin_addr.s_addr = INADDR_ANY;    // when there are few ip's for the machine. We will use always "INADDR_ANY"

    // Connects between the socket and the configuration (port and etc..)
    if (bind(m_serverSocket, (struct sockaddr*)&sa, sizeof(sa)) == SOCKET_ERROR)
        throw std::exception("Failed to bind onto the requested port");

    // Start listening for incoming requests of clients
    if (listen(m_serverSocket, SOMAXCONN) == SOCKET_ERROR)
        throw std::exception("Failed listening to requests.");
}

void Communicator::handleNewClient(SOCKET client_sock)
{
    bool run = true;
    bool send = true;
    std::string msg;
    BUFFER buf;
    BUFFER rep;
    
    int uniqueClientId = clientIdCounter.fetch_add(1);

    Client* client_handler = new Client(uniqueClientId);
    m_clients[client_sock] = client_handler;
    std::string repCode;

    try
    {
        // Send the client ID to the client
        std::string idMessage = std::to_string(MC_CLIENT_ID) + std::to_string(uniqueClientId);
        Helper::sendData(client_sock, BUFFER(idMessage.begin(), idMessage.end()));
    }
    catch (std::exception& e)
    {
        std::cerr << "Error sending initial file content to client: " << e.what() << std::endl;
        run = false;
        closesocket(client_sock);
        return;
    }

    std::string fileContent;
    std::string lengthString;
    std::string initialFileContent;

    while (run)
    {
        try
        {
            buf = Helper::getPartFromSocket(client_sock, 1024);
            if (buf.size() == 0)
            {
                closesocket(client_sock);
                run = false;
                // Handle disconnection
                handleClientDisconnect(client_sock);
                continue;
            }

            std::string newRequest(buf.begin(), buf.end());
            Action emptyAction;

            {
                // Lock the mutex before updating the file
                std::lock_guard<std::mutex> lock(m_fileMutex);
                Action reqDetail = deconstructReq(newRequest);
                reqDetail.userId = m_clients[client_sock]->getId();
                switch (reqDetail.code)
                {
                case MC_INSERT_REQUEST:
                    repCode = std::to_string(MC_INSERT_RESP);
                    break;

                case MC_DELETE_REQUEST:
                    repCode = std::to_string(MC_DELETE_RESP);
                    break;

                case MC_REPLACE_REQUEST:
                    repCode = std::to_string(MC_REPLACE_RESP);
                    break;

                case MC_INITIAL_REQUEST:
                    repCode = std::to_string(MC_INITIAL_RESP);
                    fileContent = readFromFile(".\\files\\" + reqDetail.data);
                    // Convert the length to a string with exactly 5 digits
                    lengthString = std::to_string(fileContent.length());
                    lengthString = std::string(5 - lengthString.length(), '0') + lengthString;

                    m_clients[client_sock]->setFileName(reqDetail.data);
                    emptyAction.code = MC_INITIAL_REQUEST;
                    m_lastActionMap[reqDetail.data] = emptyAction;
                    m_usersOnFile[reqDetail.data].push_back(*m_clients[client_sock]);

                    // Create the initialFileContent string
                    initialFileContent = repCode + lengthString + fileContent;
                    Helper::sendData(client_sock, BUFFER(initialFileContent.begin(), initialFileContent.end()));
                    send = false;
                    break;
                case MC_CREATE_FILE_REQUEST:
                    // Check if the file with the specified name exists
                    if (fileExists(".\\files\\" + reqDetail.data))
                    {
                        // File already exists, send an appropriate response code
                        repCode = std::to_string(MC_ERR_RESP);
                        Helper::sendData(client_sock, BUFFER(repCode.begin(), repCode.end()));
                        send = false;
                    }
                    else
                    {
                        // File doesn't exist, create it and send a success response code
                        repCode = std::to_string(MC_CREATE_FILE_RESP);
                        createFile(".\\files\\" + reqDetail.data);
                        Helper::sendData(client_sock, BUFFER(repCode.begin(), repCode.end()));
                        send = false;
                        m_clients[client_sock]->setFileName(reqDetail.data);
                        emptyAction.code = MC_CREATE_FILE_REQUEST;
                        m_lastActionMap[reqDetail.data] = emptyAction;
                        m_usersOnFile[reqDetail.data].push_back(*m_clients[client_sock]);
                    }
                    break;

                case MC_GET_FILES_REQUEST:
                    repCode = std::to_string(MC_GET_FILES_RESP);
                    send = false;
                    getFilesInDirectory(".\\files");
                    for (const auto& fileName : m_files) 
                    {
                        repCode += fileName + ";";
                    }
                    Helper::sendData(client_sock, BUFFER(repCode.begin(), repCode.end()));
                    break;

                case MC_CLOSE_FILE_REQUEST:
                    repCode = MC_CLOSE_FILE_RESP;
                    // Check if the file exists in the m_usersOnFile map
                    for (auto it = m_usersOnFile.begin(); it != m_usersOnFile.end(); ++it) {
                        // Iterate over the array of clients for each file
                        for (auto clientIt = it->second.begin(); clientIt != it->second.end(); ) {
                            if (clientIt->getId() == reqDetail.userId) {
                                clientIt = it->second.erase(clientIt);
                            }
                            else {
                                ++clientIt;
                            }
                        }
                    }
                    send = false;
                    break;

                case MC_DISCONNECT: // Handle disconnect request
                    run = false;
                    handleClientDisconnect(client_sock);
                    send = false;
                    continue;
                default:
                    // Handle the default case or throw an error
                    throw std::runtime_error("Unknown action code: " + std::to_string(reqDetail.code));
                }

                if (send)
                {
                    std::string fileName = m_clients[client_sock]->getFileName();
                    //reqDetail = adjustIndexForSync(fileName, reqDetail);
                    reqDetail.fileName = fileName;
                    updateFileOnServer(".\\files\\" + fileName, reqDetail);
                    m_lastActionMap[fileName] = reqDetail;
                    // Notify all connected clients about the file change
                    notifyAllClients(repCode + reqDetail.msg, client_sock);
                }
                send = true;
            }  // lock goes out of scope, releasing the lock


        }
        catch (...)
        {
            run = false;
            // Handle disconnection
            handleClientDisconnect(client_sock);
            continue;
        }
    }

    closesocket(client_sock);
}

Action Communicator::adjustIndexForSync(const std::string& fileName, Action reqDetail)
{
    std::string lengthString;
    std::string selectionLengthString;
    std::string indexString;

    int selectionLength;
    int length;
    std::string data;
    int newIndex;

    int newCode = reqDetail.code;
    // Check if there is a last action recorded for this file
    if (m_lastActionMap.find(fileName) != m_lastActionMap.end())
    {
        int lastUser = m_lastActionMap[fileName].userId;
        long long lastTimestamp = m_lastActionMap[fileName].timestamp;
        if (m_lastActionMap[fileName].code != MC_INITIAL_REQUEST && 
            m_lastActionMap[fileName].code != MC_CREATE_FILE_REQUEST && lastUser != reqDetail.userId
            && lastTimestamp < reqDetail.timestamp)
        {
            int lastActionCode = m_lastActionMap[fileName].code;
            int size = m_lastActionMap[fileName].size;
            int lastIndex = std::stoi(m_lastActionMap[fileName].index);

            std::string newAction = reqDetail.msg;

            std::string adjustedIndex = reqDetail.index;
            std::string updatedAction = newAction;

            newIndex = std::stoi(reqDetail.index);

            reqDetail.timestamp = getCurrentTimestamp();

            // uodate the index
            switch (lastActionCode) {
            case MC_INSERT_REQUEST:
                if (newIndex > lastIndex)
                {
                    newIndex += size;
                    adjustedIndex = std::to_string(newIndex);
                    adjustedIndex = std::string(5 - adjustedIndex.length(), '0') + adjustedIndex;
                    updatedAction = reqDetail.dataLength + reqDetail.data + adjustedIndex;
                }
                break;
            case MC_DELETE_REQUEST:
                if (newIndex > lastIndex)
                {
                    newIndex -= size;
                    adjustedIndex = std::to_string(newIndex);
                    adjustedIndex = std::string(5 - adjustedIndex.length(), '0') + adjustedIndex;
                    updatedAction = reqDetail.dataLength + adjustedIndex;
                }
                break;
            case MC_REPLACE_REQUEST:
                if (newIndex > lastIndex)
                {
                    newIndex = newIndex - std::stoi(reqDetail.selectionLength) + std::stoi(reqDetail.dataLength) ;
                    adjustedIndex = std::to_string(newIndex);
                    adjustedIndex = std::string(5 - adjustedIndex.length(), '0') + adjustedIndex;
                    updatedAction = reqDetail.selectionLength + reqDetail.dataLength + reqDetail.data + adjustedIndex;
                }
                break;
            }
            reqDetail.index = adjustedIndex;
            reqDetail.msg = updatedAction;
        }
    }
    return reqDetail;

}

void Communicator::handleClientDisconnect(SOCKET client_sock)
{
    // Clean up resources and remove the client from the map
    if (m_clients.find(client_sock) != m_clients.end())
    {
        delete m_clients[client_sock];
        m_clients.erase(client_sock);
    }

    // Notify other clients about the disconnection
    std::string disconnectMessage = std::to_string(MC_DISCONNECT) + "00000";
    notifyAllClients(disconnectMessage, client_sock);
}

Action Communicator::deconstructReq(const std::string& req) {
    std::string msgCode = req.substr(0, 3);
    std::string action = req.substr(3);

    Action newAction;
    
    std::string indexString;

    switch (std::stoi(msgCode)) 
    {
    case MC_INITIAL_REQUEST:
        newAction.data = action;
        break;
    case MC_INSERT_REQUEST:
        newAction.dataLength = action.substr(0, 5);
        newAction.size = std::stoi(newAction.dataLength);
        
        newAction.data = action.substr(5, newAction.size);
        newAction.index = action.substr(5 + newAction.size);
        break;

    case MC_DELETE_REQUEST:
        newAction.dataLength = action.substr(0, 5);
        indexString = action.substr(5);

        newAction.size = std::stoi(newAction.dataLength);
        newAction.index = indexString;
        break;

    case MC_REPLACE_REQUEST:
        newAction.selectionLength = action.substr(0, 5);
        newAction.dataLength = action.substr(5, 5);
        newAction.size = std::stoi(newAction.dataLength);
        newAction.data = action.substr(10, newAction.size);
        indexString = action.substr(10 + newAction.size);
        newAction.index = indexString;
        break;
    case MC_CREATE_FILE_REQUEST:
        newAction.data = action;
        break;
    case MC_GET_FILES_REQUEST:
        //newAction.data = action;
        break;
    case MC_CLOSE_FILE_REQUEST:
        newAction.dataLength = action.substr(0, 5);
        newAction.data = action.substr(5, std::stoi(newAction.dataLength));

    }
    newAction.timestamp = getCurrentTimestamp();
    newAction.code = std::stoi(msgCode);
    newAction.msg = action;
    return newAction;
}

void Communicator::updateFileOnServer(const std::string& filePath, const Action& reqDetail)
{
    std::fstream file(filePath, std::ios::in | std::ios::out);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file for reading/writing: " + filePath);
    }
    else {
        switch (reqDetail.code) {
        case MC_INSERT_REQUEST:
            // Insert operation
            operationHandler.insert(file, reqDetail.data, std::stoi(reqDetail.index));
            break;

        case MC_DELETE_REQUEST:
            // Delete operation
            operationHandler.deleteContent(file, std::stoi(reqDetail.dataLength), std::stoi(reqDetail.index), reqDetail.fileName);
            break;

        case MC_REPLACE_REQUEST:
            // Replace operation
            operationHandler.replace(file, std::stoi(reqDetail.selectionLength), reqDetail.data, std::stoi(reqDetail.index));
            break;

        default:
            throw std::runtime_error("Unknown action code: " + reqDetail.code);
        }

        file.close();
    }
}


void Communicator::notifyAllClients(const std::string& updatedContent, SOCKET client_sock)
{
    // Iterate through all connected clients and send them the updated content
    for (auto& sock : m_clients)
    {
        if (sock.first != client_sock)
        {
            SOCKET client_sock = sock.first;
            Helper::sendData(client_sock, BUFFER(updatedContent.begin(), updatedContent.end()));
        }
    }
}

std::string Communicator::readFromFile(const std::string& filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open file: " + filePath);
    }

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    return content;
}

bool Communicator::fileExists(const std::string& fileName)
{
    std::ifstream file(fileName);
    return file.good();
}

void Communicator::createFile(const std::string& fileName)
{
    std::ofstream file(fileName);
    if (file.is_open())
    {
        // You can optionally write some initial content to the file
        file << "Initial content of the file.\n";
        file.close();
    }
    else
    {
        std::cerr << "Error creating the file: " << fileName << std::endl;
    }
}

long long Communicator::getCurrentTimestamp() {
    auto currentTime = std::chrono::system_clock::now();
    auto duration = currentTime.time_since_epoch();

    // Convert duration to milliseconds
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration);

    // Convert milliseconds to a long long value
    return milliseconds.count();
}

void Communicator::getFilesInDirectory(const std::string& directoryPath) {
    std::vector<std::string> fileList;

    try {
        for (const auto& entry : std::filesystem::directory_iterator(directoryPath)) {
            if (std::filesystem::is_regular_file(entry)) {
                std::string fileName = entry.path().filename().string();

                if (std::find(m_files.begin(), m_files.end(), fileName) == m_files.end()) {
                    m_files.push_back(fileName);
                }
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error reading directory: " << e.what() << std::endl;
    }
}

void Communicator::startHandleRequests()
{
    SOCKET client_socket;
    bindAndListen();
    while (true)
    {
        client_socket = accept(m_serverSocket, NULL, NULL);
        if (client_socket == INVALID_SOCKET)
            throw std::exception("Recieved an invalid socket.");
        std::thread t(&Communicator::handleNewClient, this, client_socket);
        t.detach();
    }
}