#include "Communicator.h"
#pragma comment (lib, "ws2_32.lib")
#pragma comment(lib, "winhttp.lib")

#include "WSAInitializer.h"
#include <iostream>
#include <exception>

int main() {
    WSAInitializer wsaInit;
    Communicator com;
    std::thread t1(&Communicator::startHandleRequests, &com);
    t1.detach();
    std::string inp;
    while (inp != "EXIT")
    {
        std::cout << "Enter EXIT to quit." << std::endl;
        std::cin >> inp;
    }
}
