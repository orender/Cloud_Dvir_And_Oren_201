#include "Winsock2.h"
#include "container.h"
#include "dataSplitter.h"
#include <iostream>
#include "DBHelper.hpp"

#define PORT 5555
#define GOODCOMMAND 420
#define BADCOMMAND 42
int main() 
{

    WSADATA              wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    
    dataSplitter ds = dataSplitter("test.db");
    

    SOCKET listner = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    SOCKET m_syncServerSocket;
    struct sockaddr_in sa = { 0 };

    sa.sin_port = htons(PORT); // port that server will listen for
    sa.sin_family = AF_INET;   // must be AF_INET
    sa.sin_addr.s_addr = INADDR_ANY;    // when there are few ip's for the machine. We will use always "INADDR_ANY"

    // Connects between the socket and the configuration (port and etc..)
    if (bind(listner, (struct sockaddr*)&sa, sizeof(sa)) == SOCKET_ERROR)
        throw std::exception("Failed to bind onto the requested port");

    // Start listening for incoming requests of clients
    if (listen(listner, SOMAXCONN) == SOCKET_ERROR)
        throw std::exception("Failed listening to requests.");

    m_syncServerSocket = accept(listner, NULL, NULL);
    if (m_syncServerSocket == INVALID_SOCKET)
        throw std::exception("Recieved an invalid socket.");

    std::string nameLen = "";
    std::string name = "";
    std::string data = "";
    std::string a = "";
    char* buffer;
    size_t bufferSize;

    while (true)
    {
        char recvbuf[1024];
        recv(m_syncServerSocket, recvbuf, 1024, 0);
        int code = 0;
        std::string msg = "";
        readMessage(recvbuf, code, msg);

        switch (code)
        {
        case (int)saveBlobCode:
            nameLen = msg.substr(0, 3);
            name = msg.substr(3, std::stoi(nameLen));
            data = msg.substr(std::stoi(nameLen)+3);

            std::cout << msg;

            if (ds.saveNewFile(name, data))
            {

                buffer = new char[1024];
                bufferSize;
                a = "file saved successfully";
                writeMessage(GOODCOMMAND, a, buffer, bufferSize);

                send(m_syncServerSocket, buffer, bufferSize, 0);
            }
            else {

                buffer = new char[1024];
                bufferSize;
                a = "file was not saved successfully";
                writeMessage(BADCOMMAND, a, buffer, bufferSize);

                send(m_syncServerSocket, buffer, bufferSize, 0);
            }
            break;
        case (int)getBlobCode:

            buffer = new char[1024];
            bufferSize;
            a = ds.getFileData(msg);
            writeMessage(GOODCOMMAND, a, buffer, bufferSize);

            send(m_syncServerSocket, buffer, bufferSize, 0);
            break;
        case (int)deleteBlobCode:
            if (ds.deleteFile(msg))
            {
                buffer = new char[1024];
                bufferSize;
                a = "file was deleted successfully";
                writeMessage(GOODCOMMAND, a, buffer, bufferSize);

                send(m_syncServerSocket, buffer, bufferSize, 0);
            }
            else {
                buffer = new char[1024];
                bufferSize;
                a = "file was not deleted successfully";
                writeMessage(BADCOMMAND, a, buffer, bufferSize);

                send(m_syncServerSocket, buffer, bufferSize, 0);
            }
            break;
        case (int)getAllFiles:
            buffer = new char[1024];
            bufferSize;
            a = ds.getFiles();
            writeMessage(BADCOMMAND, a, buffer, bufferSize);

            send(m_syncServerSocket, buffer, bufferSize, 0);
            break;
        case (int)createNewFile:
            if (ds.saveNewFile(msg, ""))
            {

                buffer = new char[1024];
                bufferSize;
                a = "file created successfully";
                writeMessage(GOODCOMMAND, a, buffer, bufferSize);

                send(m_syncServerSocket, buffer, bufferSize, 0);
            }
            else {

                buffer = new char[1024];
                bufferSize;
                a = "file was not created successfully";
                writeMessage(BADCOMMAND, a, buffer, bufferSize);

                send(m_syncServerSocket, buffer, bufferSize, 0);
            }
            break;
        default:
            break;
        }

    }
    WSACleanup();

    return 0;
}