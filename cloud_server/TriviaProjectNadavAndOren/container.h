#pragma once
#pragma warning(disable:4996) 
#include "Winsock2.h"
#include <iostream>
#include "DataTypes.h"
#include "BinProtocol.h"
#include "DBHelper.hpp"
#include <string>
#define BLOBSIZE 50

enum messageCodes { saveBlobCode = 1, getBlobCode = 2, deleteBlobCode = 3};


class container
{
private:
	SOCKET _sock;
	SOCKADDR_IN _addr;
public:
	container(std::string ip, unsigned int port);
	~container();
	int start();
	int save(std::string SaveBlob);
	std::string getBlob(int id);
	int deleteBlob(int id);
};