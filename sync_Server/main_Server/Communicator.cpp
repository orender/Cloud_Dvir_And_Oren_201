#include "Communicator.h"
extern std::unordered_map<std::string, std::mutex> m_fileMutexes;

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
        handleClientDisconnect(client_sock);
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
                fileContent = fileOperationHandler.readFromFile(".\\files\\" + reqDetail.data);
                // Convert the length to a string with exactly 5 digits
                lengthString = std::to_string(fileContent.length());
                lengthString = std::string(5 - lengthString.length(), '0') + lengthString;

                emptyAction.code = MC_INITIAL_REQUEST;
                m_lastActionMap[".\\files\\" + reqDetail.data] = emptyAction;

                // Create the initialFileContent string
                initialFileContent = repCode + lengthString + fileContent;
                Helper::sendData(client_sock, BUFFER(initialFileContent.begin(), initialFileContent.end()));
                send = false;
                break;
            case MC_CREATE_FILE_REQUEST:
                // Check if the file with the specified name exists
                if (fileOperationHandler.fileExists(".\\files\\" + reqDetail.data + ".txt"))
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

                    // Create the mutex for the new file
                    m_fileMutexes[".\\files\\" + reqDetail.data + ".txt"];

                    fileOperationHandler.createFile(".\\files\\" + reqDetail.data + ".txt", true);
                    fileOperationHandler.createFile(".\\chats\\" + reqDetail.data + "_chat.txt", false);

                    Helper::sendData(client_sock, BUFFER(repCode.begin(), repCode.end()));
                    send = false;

                    m_clients[client_sock]->setFileName(".\\files\\" + reqDetail.data + ".txt");
                    emptyAction.code = MC_CREATE_FILE_REQUEST;
                    m_lastActionMap[".\\files\\" + reqDetail.data + ".txt"] = emptyAction;
                    m_usersOnFile[".\\files\\" + reqDetail.data + ".txt"].push_back(*m_clients[client_sock]);
                }
                break;


            case MC_GET_FILES_REQUEST:
                repCode = std::to_string(MC_GET_FILES_RESP);
                send = false;
                fileOperationHandler.getFilesInDirectory(".\\files", m_files);
                for (const auto& fileName : m_files)
                {
                    lengthString = std::to_string(fileName.length());
                    lengthString = std::string(5 - lengthString.length(), '0') + lengthString;
                    repCode += lengthString + fileName;
                }
                Helper::sendData(client_sock, BUFFER(repCode.begin(), repCode.end()));
                break;
            case MC_GET_MESSAGES_REQUEST:
                send = false;
                // Handle get messages request
                repCode = std::to_string(MC_GET_MESSAGES_RESP);
                repCode += fileOperationHandler.readFromFile(".\\chats\\" + reqDetail.data + "_chat.txt");
                Helper::sendData(client_sock, BUFFER(repCode.begin(), repCode.end()));
                break;

            case MC_GET_USERS_REQUEST:
                // Handle get users request
                repCode = std::to_string(MC_GET_USERS_RESP);
                send = false;
                // Get the list of users logged into the file
                for (const auto& user : m_usersOnFile[".\\files\\" + reqDetail.data]) {
                    lengthString = std::to_string(user.getId());
                    lengthString = std::string(5 - lengthString.length(), '0') + lengthString;
                    repCode += lengthString;
                }
                Helper::sendData(client_sock, BUFFER(repCode.begin(), repCode.end()));
                break;

            case MC_POST_MSG_REQUEST:
                // Handle post message request
                repCode = std::to_string(MC_POST_MSG_RESP);
                updateChatFileOnServer(".\\chats\\" + reqDetail.fileName + "_chat.txt", reqDetail);
                notifyAllClients(repCode + reqDetail.dataLength + reqDetail.data + reqDetail.index
                    , client_sock);
                send = false;
                break;

            case MC_JOIN_FILE_REQUEST:
                repCode = std::to_string(MC_JOIN_FILE_RESP);
                m_clients[client_sock]->setFileName(".\\files\\" + reqDetail.data);

                // Create the mutex for the file if it doesn't exist
                m_fileMutexes.try_emplace(".\\files\\" + reqDetail.data);

                m_usersOnFile[".\\files\\" + reqDetail.data].push_back(*m_clients[client_sock]);
                notifyAllClients(repCode + std::to_string(reqDetail.userId), client_sock);
                send = false;
                break;
            case MC_LEAVE_FILE_REQUEST:
                repCode = std::to_string(MC_LEAVE_FILE_RESP);
                lengthString = std::to_string(reqDetail.userId);
                lengthString = std::string(5 - lengthString.length(), '0') + lengthString;
                repCode += lengthString;
                Helper::sendData(client_sock, BUFFER(repCode.begin(), repCode.end()));
                repCode = std::to_string(MC_LEAVE_FILE_RESP);

                reqDetail.fileName = m_clients[client_sock]->getFileName();

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

                lengthString = std::to_string(reqDetail.userId);
                lengthString = std::string(5 - lengthString.length(), '0') + lengthString;
                repCode += lengthString;
                notifyAllClients(repCode, client_sock);
                send = false;

                // Check if the user leaving was the last one
                if (m_usersOnFile[reqDetail.fileName].empty()) {
                    // Delete the mutex and remove the file from m_usersOnFile
                    m_fileMutexes.erase(reqDetail.fileName);
                    m_usersOnFile.erase(reqDetail.fileName);
                }
                m_clients[client_sock]->setFileName("");
                break;
            case MC_DISCONNECT: // Handle disconnect request
                run = false;
                handleClientDisconnect(client_sock);
                send = false;
                continue;
            default:
                // Handle the default case or throw an error
                throw std::runtime_error("Unknown action code: " + reqDetail.msg);
            }

            if (send)
            {
                {
                    std::string fileName = m_clients[client_sock]->getFileName();
                    // Lock the mutex before updating the file
                    std::lock_guard<std::mutex> lock(m_fileMutexes[fileName]);

                    reqDetail = adjustIndexForSync(fileName, reqDetail);
                    reqDetail.fileName = fileName;
                    updateFileOnServer(fileName, reqDetail);
                    notifyAllClients(repCode + reqDetail.msg, client_sock);

                    reqDetail.timestamp = getCurrentTimestamp();
                    m_lastActionMap[fileName] = reqDetail;
                }// lock goes out of scope, releasing the lock

            }
            send = true;


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
            m_lastActionMap[fileName].code != MC_CREATE_FILE_REQUEST &&
            lastUser != reqDetail.userId && 
            lastTimestamp > reqDetail.timestamp)
        {
            int lastActionCode = m_lastActionMap[fileName].code;
            int size = m_lastActionMap[fileName].size;
            int lastIndex = std::stoi(m_lastActionMap[fileName].index);

            std::string newAction = reqDetail.msg;

            std::string adjustedIndex = reqDetail.index;
            std::string updatedAction = newAction;

            newIndex = std::stoi(reqDetail.index);

            //reqDetail.timestamp = getCurrentTimestamp();

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
    // Check if the client is associated with a file
    if (m_clients.find(client_sock) != m_clients.end())
    {
        Client* disconnectedClient = m_clients[client_sock];

        // Check if the client is inside a file
        if (disconnectedClient->getFileName() != "")
        {
            std::string fileName = disconnectedClient->getFileName();

            // Remove the client from the file's user list
            auto it = m_usersOnFile.find(fileName);
            if (it != m_usersOnFile.end())
            {
                it->second.erase(std::remove_if(it->second.begin(), it->second.end(),
                    [disconnectedClient](const Client& client) {
                        return client.getId() == disconnectedClient->getId();
                    }), it->second.end());

                if (m_usersOnFile[fileName].empty()) {
                    // Delete the mutex and remove the file from m_usersOnFile
                    m_fileMutexes.erase(fileName);
                    m_usersOnFile.erase(fileName);
                }

                std::string lengthString = std::to_string(disconnectedClient->getId());
                lengthString = std::string(5 - lengthString.length(), '0') + lengthString;
                // Notify other clients in the same file about the disconnection
                std::string leaveFileMessage = std::to_string(MC_LEAVE_FILE_RESP) + lengthString;
                notifyAllClients(leaveFileMessage, client_sock);
            }
        }

        // Clean up resources and remove the client from the map
        delete disconnectedClient;
        m_clients.erase(client_sock);
    }
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
    case MC_GET_MESSAGES_REQUEST:
        newAction.data = action;
        break;
    case MC_GET_USERS_REQUEST:
        newAction.data = action;
        break;
    case MC_POST_MSG_REQUEST:
        newAction.fileNameLength = action.substr(0,5);
        newAction.size = std::stoi(newAction.fileNameLength);
        newAction.fileName = action.substr(5, newAction.size);
        newAction.dataLength = action.substr(5 + newAction.size, 5);
        newAction.data = action.substr(10 + newAction.size, std::stoi(newAction.dataLength));
        newAction.index = action.substr(10 + newAction.size + std::stoi(newAction.dataLength), 5); // use this as user id
        break;
    case MC_JOIN_FILE_REQUEST:
        newAction.dataLength = action.substr(0, 5);
        newAction.data = action.substr(5, std::stoi(newAction.dataLength));
        break;
    case MC_LEAVE_FILE_REQUEST:
        newAction.data = action.substr(0, 5);
        break;
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

void Communicator::updateChatFileOnServer(const std::string& filePath, const Action& reqDetail)
{
    std::ofstream file(filePath, std::ios::app);  // Open file in append mode

    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file for writing: " + filePath);
    }
    else {
        // Format the message
        std::string message = reqDetail.dataLength + reqDetail.data + reqDetail.index ;

        // Append the message to the file
        file << message;

        // Close the file
        file.close();
    }
}

void Communicator::notifyAllClients(const std::string& updatedContent, SOCKET client_sock)
{
    // Iterate through all connected clients and send them the updated content
    for (auto& sock : m_clients)
    {
        if (sock.first != client_sock && 
            m_clients[client_sock]->getFileName() == m_clients[sock.first]->getFileName())
        {
            SOCKET client_sock = sock.first;
            Helper::sendData(client_sock, BUFFER(updatedContent.begin(), updatedContent.end()));
        }
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