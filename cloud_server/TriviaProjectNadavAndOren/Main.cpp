#include "Winsock2.h"
#include <iostream>

#pragma warning(disable:4996) 

int main() {
    WSADATA              wsaData;

    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET               SendingSocketContainer1;
    SOCKET               SendingSocketContainer2;
    SOCKET               SendingSocketContainer3;

    // Server/receiver address

    SOCKADDR_IN          ServerAddrContainer1, ThisSenderInfoContainer1;
    SOCKADDR_IN          ServerAddrContainer2, ThisSenderInfoContainer2;
    SOCKADDR_IN          ServerAddrContainer3, ThisSenderInfoContainer3;

    // Server/receiver port to connect to

    unsigned int         PortContainer1 = 12345;
    unsigned int         PortContainer2 = 12350;
    unsigned int         PortContainer3 = 12355;


    // Be careful with the array bound, provide some checking mechanism...

    char sendbuf[1024] = "This is a test string from sender";

    int BytesSent, nlen;





    SendingSocketContainer1 = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    SendingSocketContainer2 = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    SendingSocketContainer3 = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    ServerAddrContainer1.sin_family = AF_INET;
    ServerAddrContainer2.sin_family = AF_INET;
    ServerAddrContainer3.sin_family = AF_INET;

    ServerAddrContainer1.sin_port = htons(PortContainer1);
    ServerAddrContainer2.sin_port = htons(PortContainer2);
    ServerAddrContainer3.sin_port = htons(PortContainer3);

    ServerAddrContainer1.sin_addr.s_addr = inet_addr("127.0.0.1");
    ServerAddrContainer2.sin_addr.s_addr = inet_addr("127.0.0.1");
    ServerAddrContainer3.sin_addr.s_addr = inet_addr("127.0.0.1");



    connect(SendingSocketContainer1, (SOCKADDR*)&ServerAddrContainer1, sizeof(ServerAddrContainer1));
    connect(SendingSocketContainer2, (SOCKADDR*)&ServerAddrContainer2, sizeof(ServerAddrContainer2));
    connect(SendingSocketContainer3, (SOCKADDR*)&ServerAddrContainer3, sizeof(ServerAddrContainer3));

    BytesSent = send(SendingSocketContainer1, sendbuf, strlen(sendbuf), 0);

    printf("Client: send() is OK - bytes sent: %ld\n", BytesSent);

    memset(&ThisSenderInfoContainer1, 0, sizeof(ThisSenderInfoContainer1));

    nlen = sizeof(SendingSocketContainer1);



    getsockname(SendingSocketContainer1, (SOCKADDR*)&ThisSenderInfoContainer1, &nlen);

    printf("Client: Sender IP(s) used: %s\n", inet_ntoa(ThisSenderInfoContainer1.sin_addr));

    printf("Client: Sender port used: %d\n", htons(ThisSenderInfoContainer1.sin_port));

    printf("Client: Those bytes represent: \"%s\"\n", sendbuf);


    BytesSent = send(SendingSocketContainer2, sendbuf, strlen(sendbuf), 0);

    printf("Client: send() is OK - bytes sent: %ld\n", BytesSent);

    memset(&ThisSenderInfoContainer2, 0, sizeof(ThisSenderInfoContainer2));

    nlen = sizeof(SendingSocketContainer2);



    getsockname(SendingSocketContainer2, (SOCKADDR*)&ThisSenderInfoContainer2, &nlen);

    printf("Client: Sender IP(s) used: %s\n", inet_ntoa(ThisSenderInfoContainer2.sin_addr));

    printf("Client: Sender port used: %d\n", htons(ThisSenderInfoContainer2.sin_port));

    printf("Client: Those bytes represent: \"%s\"\n", sendbuf);

    BytesSent = send(SendingSocketContainer3, sendbuf, strlen(sendbuf), 0);

    printf("Client: send() is OK - bytes sent: %ld\n", BytesSent);

    memset(&ThisSenderInfoContainer3, 0, sizeof(ThisSenderInfoContainer3));

    nlen = sizeof(SendingSocketContainer3);



    getsockname(SendingSocketContainer3, (SOCKADDR*)&ThisSenderInfoContainer3, &nlen);

    printf("Client: Sender IP(s) used: %s\n", inet_ntoa(ThisSenderInfoContainer3.sin_addr));

    printf("Client: Sender port used: %d\n", htons(ThisSenderInfoContainer3.sin_port));

    printf("Client: Those bytes represent: \"%s\"\n", sendbuf);


    char recvbuf[1024];

    recv(SendingSocketContainer1, recvbuf, 1024, 0);
    printf("Server: \"%s\"\n", recvbuf);
    recv(SendingSocketContainer2, recvbuf, 1024, 0);
    printf("Server: \"%s\"\n", recvbuf);
    recv(SendingSocketContainer3, recvbuf, 1024, 0);
    printf("Server: \"%s\"\n", recvbuf);

    printf("Server: \"%s\"\n", recvbuf);

    shutdown(SendingSocketContainer1, SD_SEND);
    shutdown(SendingSocketContainer2, SD_SEND);
    shutdown(SendingSocketContainer3, SD_SEND);

    closesocket(SendingSocketContainer1);
    closesocket(SendingSocketContainer2);
    closesocket(SendingSocketContainer3);



    WSACleanup();

    return 0;
}