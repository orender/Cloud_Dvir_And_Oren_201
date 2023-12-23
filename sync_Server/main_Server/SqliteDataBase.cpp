#include "SqliteDataBase.h"

int callback_data(void* data, int argc, char** argv, char** azColName)
{
	std::list<std::string>* list_data = (std::list<std::string>*)data;

	for (int i = 0; i < argc; i++) {
		list_data->push_back(argv[i]);
	}
	return 0;
}

int callback_users(void* data, int argc, char** argv, char** azColName)
{
	std::list<Client>* list_users = (std::list<Client>*)data;
	Client user;

	for (int i = 0; i < argc; i++) {
		if (std::string(azColName[i]) == "user_name") {
			user.setUsername(argv[i]);
		}
		else if (std::string(azColName[i]) == "mail") {
			user.setEmail(argv[i]);
		}
		else if (std::string(azColName[i]) == "password") {
			user.setPass(argv[i]);
		}
	}
	list_users->push_back(user);
	return 0;
}

int callback_chats(void* data, int argc, char** argv, char** azColName)
{
	std::list<Chat>* list_chats = (std::list<Chat>*)data;
	Chat chat;

	for (int i = 0; i < argc; i++) {
		if (std::string(azColName[i]) == "fileName") {
			chat.fileName = argv[i];
		}
		else if (std::string(azColName[i]) == "data") {
			chat.data = argv[i];
		}
	}
	list_chats->push_back(chat);
	return 0;
}

bool SqliteDataBase::send(sqlite3* db, std::string msg)
{
	const char* sqlStatement = msg.c_str();
	char* errMessage = nullptr;
	int res = sqlite3_exec(db, sqlStatement, nullptr, nullptr, &errMessage);
	if (res != SQLITE_OK)
		return false;

	return true;
}

bool SqliteDataBase::send_users(sqlite3* db, std::string msg, std::list<Client>* users)
{
	const char* sqlStatement = msg.c_str();
	char* errMessage = nullptr;
	int res = sqlite3_exec(db, sqlStatement, callback_users, users, &errMessage);
	if (res != SQLITE_OK)
		return false;

	return true;
}

bool SqliteDataBase::send_chats(sqlite3* db, std::string msg, std::list<Chat>* chats)
{
	const char* sqlStatement = msg.c_str();
	char* errMessage = nullptr;
	int res = sqlite3_exec(db, sqlStatement, callback_chats, chats, &errMessage);
	if (res != SQLITE_OK)
		return false;

	return true;
}

bool SqliteDataBase::send_data(sqlite3* db, std::string msg, std::list<std::string>* data)
{
	const char* sqlStatement = msg.c_str();
	char* errMessage = nullptr;
	int res = sqlite3_exec(db, sqlStatement, callback_data, data, &errMessage);
	if (res != SQLITE_OK)
		return false;

	return true;
}


bool SqliteDataBase::open()
{
	std::string dbFileName = "syncDB.sqlite";
	int file_exist = _access(dbFileName.c_str(), 0);
	int res = sqlite3_open(dbFileName.c_str(), &_db);

	if (res != SQLITE_OK) {
		_db = nullptr;
		std::cout << "Failed to open DB" << std::endl;
		return -1;
	}
	if (file_exist != 0) {
		std::string msg;

		msg = "CREATE TABLE users ("
			" id INTEGER PRIMARY KEY AUTOINCREMENT,"
			" user_name TEXT UNIQUE NOT NULL,"
			" password TEXT NOT NULL,"
			" mail TEXT NOT NULL);";
		send(_db, msg);
		msg = "CREATE TABLE chats ("
			" id INTEGER PRIMARY KEY AUTOINCREMENT,"
			" fileName TEXT UNIQUE NOT NULL,"
			" data TEXT NOT NULL);";
		send(_db, msg);


	}
	return true;
}

bool SqliteDataBase::close()
{
	sqlite3_close(_db);
	_db = nullptr;
	return true;
}

std::list<Client> SqliteDataBase::getAllUsers()
{
	std::string msg = "SELECT * FROM users;";
	std::list<Client> listOfUsers = {};
	send_users(_db, msg, &listOfUsers);

	return listOfUsers;
}

int SqliteDataBase::getUserId(std::string username)
{
	std::string msg = "SELECT id FROM users WHERE user_name = \'" + username + "\';";
	std::list<std::string> list_data;
	send_data(_db, msg, &list_data);
	int id;

	if (list_data.empty())
	{
		throw std::exception("Failed to find user id");
	}
	for (const auto& per : list_data)
	{
		id = atoi(per.c_str());
	}
	return id;
}

std::string SqliteDataBase::getUserName(std::string username)
{
	std::list<Client> users_list = getAllUsers();

	if (!users_list.empty())
	{
		for (auto user : users_list)
		{
			if (user.getUsername() == username || user.getEmail() == username)
			{
				return user.getUsername();
			}
		}
	}
	return "";
}

std::string SqliteDataBase::getEmail(std::string username)
{
	std::list<Client> users_list = getAllUsers();

	if (!users_list.empty())
	{
		for (auto user : users_list)
		{
			if (user.getUsername() == username || user.getEmail() == username)
			{
				return user.getEmail();
			}
		}
	}
	return "";
}

bool SqliteDataBase::doesUserExist(std::string username)
{
	std::list<Client> users_list = getAllUsers();

	if (!users_list.empty())
	{
		for (auto user : users_list)
		{
			if (user.getUsername() == username || user.getEmail() == username)
			{
				return true;
			}
		}
	}
	return false;
}

bool SqliteDataBase::doesPasswordMatch(std::string username, std::string password)
{
	std::list<Client> users_list = getAllUsers();

	if (!users_list.empty())
	{
		for (auto user : users_list)
		{
			if (user.getUsername() == username && user.getPass() == password || user.getEmail() == username && user.getPass() == password)
			{
				return true;
			}
		}
	}
	return false;
}


bool SqliteDataBase::addNewUser(std::string username, std::string password, std::string email)
{
	std::string msg;

	msg = "INSERT INTO users (user_name, password, mail) "
		"VALUES (\'" + username + "\', \'" + password + "\', \'" + email + "\');";

	return send(_db, msg);
}

void SqliteDataBase::UpdateChat(const std::string& fileName, const std::string& data)
{
	std::string msg;

	msg = "UPDATE chats SET data = data || \'" + data + "\' WHERE fileName = \'" + fileName + "\';";

	send(_db, msg);
}

void SqliteDataBase::createChat(const std::string& fileName)
{
	std::string msg;

	msg = "INSERT INTO chats (fileName, data) VALUES (\'" + fileName + "\', ''); ";

	send(_db, msg);
}

void SqliteDataBase::DeleteChat(const std::string& fileName)
{
	std::string msg;

	msg = "DELETE FROM chats WHERE fileName = \'" + fileName + "\';";

	send(_db, msg);
}

std::string SqliteDataBase::GetChatData(const std::string& fileName)
{
	std::string msg;
	std::list<Chat> chatList; // This list will store the result data

	// Assuming 'fileName' is a unique identifier in the 'chats' table

	msg = "SELECT * FROM chats WHERE fileName = \'" + fileName + "\';";

	send_chats(_db, msg, &chatList);

	for (const Chat& data : chatList) {
		if (data.fileName == fileName)
		{
			return data.data;
		}
	}
}