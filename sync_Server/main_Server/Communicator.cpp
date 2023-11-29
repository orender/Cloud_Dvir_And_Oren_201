#include "Communicator.h"

Communicator::Communicator()
{
    m_fileContent = readFromFile(m_file_name);
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

void Communicator::handleNewClient(SOCKET client_sock) {
    bool run = true;
    std::string msg;
    BUFFER buf;
    BUFFER rep;
    Client* client_handler = new Client();
    m_clients[client_sock] = client_handler;

    // Send the initial file content to the client when they join
    try {
        std::string initialFileContent = readFromFile(m_file_name);
        BUFFER initialFileContentBuffer(initialFileContent.begin(), initialFileContent.end());
        sendData(client_sock, initialFileContentBuffer);
    }
    catch (std::exception& e) {
        std::cerr << "Error sending initial file content to client: " << e.what() << std::endl;
        run = false;
        closesocket(client_sock);
        return;
    }

    while (run) {
        try {
            buf = getPartFromSocket(client_sock, 1024);
            if (buf.size() == 0) {
                closesocket(client_sock);
                run = false;
                continue;
            }

            std::string receivedMessage(buf.begin(), buf.end());

            // Parse the received message to get input and index
            size_t commaPos = receivedMessage.find(",");
            if (commaPos != std::string::npos) {
                std::string input = receivedMessage.substr(0, commaPos);
                size_t index = std::stoi(receivedMessage.substr(commaPos + 1));

                // Lock the mutex before updating the file
                {
                    std::lock_guard<std::mutex> lock(m_fileMutex);

                    // Update the content at the specified index
                    if (index < getFileContent().size()) {
                        getFileContent().insert(getFileContent().begin() + index, input.begin(), input.end());
                        notifyAllClients(std::to_string(index) + "," + input);
                    }
                }  // lock goes out of scope, releasing the lock
            }
        }
        catch (...) {
            run = false;
            continue;
        }
    }

    closesocket(client_sock);
}
void Communicator::updateFileOnServer(const std::string& filePath, const std::string& content)
{
    std::ofstream file(filePath);
    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open file for writing: " + filePath);
    }

    file << content;
    file.close();
}

void Communicator::notifyAllClients(const std::string& updatedContent)
{
    // Iterate through all connected clients and send them the updated content
    for (auto& pair : m_clients)
    {
        SOCKET client_sock = pair.first;
        sendData(client_sock, BUFFER(updatedContent.begin(), updatedContent.end()));
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

void Communicator::sendData(const SOCKET sc, const BUFFER message)
{
    const char* data = message.data();

    if (send(sc, data, message.size(), 0) == INVALID_SOCKET)
    {
        throw std::exception("Error while sending message to client");
    }
}

BUFFER Communicator::getPartFromSocket(const SOCKET sc, const int bytesNum)
{
    return getPartFromSocket(sc, bytesNum, 0);
}

BUFFER Communicator::getPartFromSocket(const SOCKET sc, const int bytesNum, const int flags)
{
    if (bytesNum == 0)
    {
        return BUFFER();
    }

    BUFFER recieved(bytesNum);
    int bytes_recieved = recv(sc, &recieved[0], bytesNum, flags);
    if (bytes_recieved == INVALID_SOCKET)
    {
        std::string s = "Error while recieving from socket: ";
        s += std::to_string(sc);
        throw std::exception(s.c_str());
    }
    recieved.resize(bytes_recieved);
    return recieved;
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