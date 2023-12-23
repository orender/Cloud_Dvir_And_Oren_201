#include "Winsock2.h"
#include "container.h"
#include <iostream>

int main() {
    WSADATA              wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    unsigned int         PortContainer1 = 12345;
    unsigned int         PortContainer2 = 12350;
    unsigned int         PortContainer3 = 12355;


    container c1 = container("127.0.0.1", PortContainer1);
    while (c1.start() == -1) {
    }
    c1.save("000001ok");
    c1.getBlob(1);
    c1.deleteBlob(1);

    WSACleanup();

    return 0;
}