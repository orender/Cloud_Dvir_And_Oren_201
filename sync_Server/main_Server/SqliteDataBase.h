#pragma once
#include "IDatabase.h"
#include "Client.h"



class SqliteDataBase : public IDatabase
{
public:
	SqliteDataBase() = default;
	~SqliteDataBase() = default;

	bool open() override;
	bool close() override;
	bool doesUserExist(std::string username) override;
	bool doesPasswordMatch(std::string username, std::string password) override;

	bool addNewUser(std::string username, std::string password, std::string email) override;
	int getUserId(std::string username) override;
	std::string getUserName(std::string username) override;
	std::string getEmail(std::string username) override;
	std::list<Client> getAllUsers() override;
	std::string GetChatData(const std::string& fileName) override;

	void UpdateChat(const std::string& fileName, const std::string& data) override;
	void createChat(const std::string& fileName) override;
	void DeleteChat(const std::string& fileName) override;

private:
	sqlite3* _db;

	bool send(sqlite3* db, std::string msg);
	bool send_users(sqlite3* db, std::string msg, std::list<Client>* users);
	bool send_data(sqlite3* db, std::string msg, std::list<std::string>* data);
	bool send_chats(sqlite3* db, std::string msg, std::list<Chat>* data);
};

