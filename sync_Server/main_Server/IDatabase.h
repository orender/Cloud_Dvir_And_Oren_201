#pragma once
#include <string>
#include "sqlite3.h"
#include <io.h>
#include <list>
#include <vector>
#include "Client.h"

struct Chat
{
	std::string fileName;
	std::string data;
};

class IDatabase
{
public:
	virtual ~IDatabase() = default;
	virtual bool open() = 0;
	virtual bool close() = 0;
	virtual bool doesUserExist(std::string username) = 0;
	virtual bool doesPasswordMatch(std::string username, std::string password) = 0;

	virtual bool addNewUser(std::string username, std::string password, std::string email) = 0;
	virtual int getUserId(std::string username) = 0;
	virtual int getIndex(std::string username, std::string fileName) = 0;
	virtual std::string getUserName(std::string username) = 0;
	virtual std::string getEmail(std::string username) = 0;
	virtual std::list<Client> getAllUsers() = 0;
	virtual std::string GetChatData(const std::string& fileName) = 0;

	virtual void UpdateChat(const std::string& fileName, const std::string& data) = 0;
	virtual void createChat(const std::string& fileName) = 0;
	virtual void DeleteChat(const std::string& fileName) = 0;
	virtual void updateIndex(std::string username, std::string fileName, int index) = 0;
	virtual void addIndex(std::string username, std::string fileName) = 0;
	virtual void deleteIndex(std::string username, std::string fileName) = 0;
};
