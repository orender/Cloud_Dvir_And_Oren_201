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
	// Default constructor
	container();
	container(std::string ip, unsigned int port);
	~container();
	int start();
	int save(std::string SaveBlob, std::string id);
	std::string getBlob(std::string id);
	int deleteBlob(std::string id);
};