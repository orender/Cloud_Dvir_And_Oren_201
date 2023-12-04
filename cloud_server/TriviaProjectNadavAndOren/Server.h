#pragma once
#include "Communicator.h"
class Server {
private:
	Communicator m_communicator;
	WSADATA m_wsaData;
public:
	Server();
	~Server();
	void run();
};