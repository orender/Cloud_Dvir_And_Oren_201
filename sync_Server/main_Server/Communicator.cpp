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
    std::string msg;
    BUFFER buf;
    BUFFER rep;
    
    int uniqueClientId = clientIdCounter.fetch_add(1);

    Client* client_handler = new Client(uniqueClientId);
    m_clients[client_sock] = client_handler;
    std::string repCode;

    // Send the initial file content to the client when they join
    try
    {

        // Send the client ID to the client
        std::string idMessage = std::to_string(MC_CLIENT_ID) + std::to_string(uniqueClientId);
        Helper::sendData(client_sock, BUFFER(idMessage.begin(), idMessage.end()));

        buf = Helper::getPartFromSocket(client_sock, 1024);
        std::string req(buf.begin(), buf.end());

        // Extract the message code from the received req string
        std::string receivedMessageCode = req.substr(0, 3);

        if (receivedMessageCode == std::to_string(MC_INITIAL_REQUEST))
        {
            std::string fileContent = readFromFile(m_file_name);
            // Convert the length to a string with exactly 5 digits
            std::string lengthString = std::to_string(fileContent.length());
            lengthString = std::string(5 - lengthString.length(), '0') + lengthString;

            // Create the initialFileContent string
            std::string initialFileContent = std::to_string(MC_INITIAL_RESP) + lengthString + fileContent;

            BUFFER initialFileContentBuffer(initialFileContent.begin(), initialFileContent.end());
            Helper::sendData(client_sock, initialFileContentBuffer);
        }

    }
    catch (std::exception& e)
    {
        std::cerr << "Error sending initial file content to client: " << e.what() << std::endl;
        run = false;
        closesocket(client_sock);
        return;
    }

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

            {
                // Lock the mutex before updating the file
                std::lock_guard<std::mutex> lock(m_fileMutex);
                std::pair<std::string, std::string> reqDetail = deconstructReq(newRequest);
                
                switch (std::stoi(reqDetail.first)) {
                case MC_INSERT_REQUEST:
                    repCode = std::to_string(MC_INSERT_RESP);
                    break;

                case MC_DELETE_REQUEST:
                    repCode = std::to_string(MC_DELETE_RESP);
                    break;

                case MC_REPLACE_REQUEST:
                    repCode = std::to_string(MC_REPLACE_RESP);
                    break;

                case MC_DISCONNECT: // Handle disconnect request
                    run = false;
                    handleClientDisconnect(client_sock);
                    continue;

                default:
                    // Handle the default case or throw an error
                    throw std::runtime_error("Unknown action code: " + reqDetail.first);
                }
                updateFileOnServer(m_file_name, reqDetail.first, reqDetail.second);
                // Notify all connected clients about the file change
                notifyAllClients(repCode + reqDetail.second, client_sock);
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

// New method to handle client disconnection
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

std::pair<std::string, std::string> Communicator::deconstructReq(const std::string& req) {
    std::string msgCode = req.substr(0, 3);
    std::string action = req.substr(3);

    return std::make_pair(msgCode, action);
}

void Communicator::updateFileOnServer(const std::string& filePath, const std::string& code, const std::string& action) {
    std::fstream file(filePath, std::ios::in | std::ios::out);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file for reading/writing: " + filePath);
    }
    else {
        switch (std::stoi(code)) {
        case MC_INSERT_REQUEST:
            // Insert operation
            insert(file, action);
            break;

        case MC_DELETE_REQUEST:
            // Delete operation
            deleteContent(file, action);
            break;

        case MC_REPLACE_REQUEST:
            // Replace operation
            replace(file, action);
            break;

        default:
            throw std::runtime_error("Unknown action code: " + code);
        }

        file.close();
    }
}

void Communicator::replace(std::fstream& file, const std::string& action)
{
    // Extract information from the action string
    std::string selectionLengthString = action.substr(0, 5);
    std::string replacementTextLengthString = action.substr(5, 5);
    std::string replacementText = action.substr(10, std::stoi(replacementTextLengthString));
    std::string indexString = action.substr(10 + std::stoi(replacementTextLengthString));

    int selectionLength = std::stoi(selectionLengthString);
    int replacementTextLength = std::stoi(replacementTextLengthString);
    int index = std::stoi(indexString);

    // Move the file pointer to the replacement index
    file.seekp(index + selectionLength, std::ios::beg);

    // Copy everything after the selection to copyText
    std::stringstream copyBuffer;
    copyBuffer << file.rdbuf();
    std::string copyText = copyBuffer.str();

    // Move the file pointer back to the replacement index
    file.seekp(index, std::ios::beg);

    // Remove everything after the index
    file.write(std::string(selectionLength + copyText.length(), '\0').c_str(), selectionLength + copyText.length());

    // Move the file pointer back to the replacement index
    file.seekp(index, std::ios::beg);

    // Write the replacementText
    file << replacementText + copyText;
}

void Communicator::insert(std::fstream& file, const std::string& action)
{
    // Extract length, data, and index from the action string
    std::string lengthString = action.substr(0, 5);
    int length = std::stoi(lengthString);

    std::string data = action.substr(5, length);
    int index = std::stoi(action.substr(5 + length));

    // Move the file pointer to the insertion index
    file.seekg(index, std::ios::beg);

    // Read the content after the insertion point
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string originalContentAfterIndex = buffer.str();

    // Move the file pointer back to the insertion index
    file.seekp(index, std::ios::beg);

    // Insert the new string
    file << data + originalContentAfterIndex;
}


void Communicator::deleteContent(std::fstream& file, const std::string& action)
{
    // Open the file in input/output mode
    file.open(m_file_name, std::ios::in | std::ios::out);

    // Check if the file is open
    if (!file.is_open()) {
        throw std::runtime_error("Unknown file: " + m_file_name);
        return;
    }

    // Read the entire content of the file into a string
    std::stringstream contentBuffer;
    contentBuffer << file.rdbuf();
    std::string fileContent = contentBuffer.str();

    // Extract length and index from the action string
    std::string lengthString = action.substr(0, 5);
    std::string indexString = action.substr(5);

    int lengthToDelete = std::stoi(lengthString);
    int index = std::stoi(indexString);

    // Check if the deletion index is within the bounds of the file content
    if (index >= 0 && index < fileContent.length()) {
        // Modify the content in memory
        fileContent.erase(index, lengthToDelete);

        // Clear the content of the file
        file.seekp(0, std::ios::beg);
        file.close();

        // Open the file again to truncate it
        file.open(m_file_name, std::ios::out | std::ios::trunc);

        // Write the modified content back to the file
        file << fileContent;
    }

    // Close the file
    file.close();
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