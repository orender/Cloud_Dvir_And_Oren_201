#include "container.h"

container::container(std::string ip, unsigned int port)
{
    //initialize socket
	_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    
    //set up info for connection
    _addr.sin_family = AF_INET;
    _addr.sin_port = htons(port);
    _addr.sin_addr.s_addr = inet_addr(ip.c_str());
}

container::~container()
{
    char* buffer = new char[1024];
    size_t bufferSize;
    writeMessage(69, "yuno miles == the goat", buffer, bufferSize);


    send(_sock, buffer, bufferSize, 0);
    shutdown(_sock, SD_SEND);
    closesocket(_sock);
}

int container::start()
{
    return connect(_sock, (SOCKADDR*)&_addr, sizeof(_addr));
}

int container::save(std::string SaveBlob)
{
    char* buffer = new char[1024];
    size_t bufferSize;
    writeMessage(saveBlobCode, SaveBlob, buffer, bufferSize);


    send(_sock, buffer, bufferSize, 0);

    char recvbuf[1024];
    recv(_sock, recvbuf, 1024, 0);
    int code = 0;
    std::string msg = "";
    readMessage(recvbuf, code, msg);
    
    std::cout << code << ", " << msg << std::endl;

    return 0;
}

std::string container::getBlob(int id)
{
    char* buffer = new char[1024];
    size_t bufferSize;
    writeMessage(getBlobCode, std::to_string(id), buffer, bufferSize);


    send(_sock, buffer, bufferSize, 0);

    char recvbuf[1024];
    recv(_sock, recvbuf, 1024, 0);
    int code = 0;
    std::string msg = "";
    readMessage(recvbuf, code, msg);

    std::cout << code << ", " << msg << std::endl;

    return msg;
}

int container::deleteBlob(int id)
{
    char* buffer = new char[1024];
    size_t bufferSize;
    writeMessage(deleteBlobCode, std::to_string(id), buffer, bufferSize);


    send(_sock, buffer, bufferSize, 0);

    char recvbuf[1024];
    recv(_sock, recvbuf, 1024, 0);
    int code = 0;
    std::string msg = "";
    readMessage(recvbuf, code, msg);

    std::cout << code << ", " << msg << std::endl;

    return 0;
}
