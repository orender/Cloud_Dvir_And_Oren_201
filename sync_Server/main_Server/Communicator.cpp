#include "Communicator.h"
extern std::unordered_map<std::string, std::mutex> m_fileMutexes;

Communicator::Communicator()
{
	// this server use TCP. that why SOCK_STREAM & IPPROTO_TCP
	// if the server use UDP we will use: SOCK_DGRAM & IPPROTO_UDP
	m_serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_serverSocket == INVALID_SOCKET)
		throw std::exception("Failed to initialize server socket.");
}

// Destructor
Communicator::~Communicator() {
	try {
		closesocket(m_serverSocket);
	}
	catch (...) {}
}

void Communicator::setDB(IDatabase* db)
{
	m_database = db;
}

void Communicator::bindAndListen()
{
	struct sockaddr_in sa = { 0 };

	sa.sin_port = htons(PORT); // port that server will listen for
	sa.sin_family = AF_INET;   // must be AF_INET
	sa.sin_addr.s_addr = INADDR_ANY;    // when there are few ip's for the machine. We will use always "INADDR_ANY"

	// Connects between the socket and the configuration (port and etc..)
	if (bind(m_serverSocket, (struct sockaddr*)&sa, sizeof(sa)) == SOCKET_ERROR)
		throw std::exception("Failed to bind onto the requested port");

	// Start listening for incoming requests of clients
	if (listen(m_serverSocket, SOMAXCONN) == SOCKET_ERROR)
		throw std::exception("Failed listening to requests.");
}

void Communicator::handleNewClient(SOCKET client_sock)
{
	bool run = true;
	bool send = true;
	std::string msg;
	BUFFER buf;
	BUFFER rep;
	
	std::string repCode;
	std::string fileContent;
	std::string lengthString;
	std::string initialFileContent;

	while (run)
	{
		try
		{
			buf = Helper::getPartFromSocket(client_sock, 1024);
			if (buf.size() == 0)
			{
				closesocket(client_sock);
				run = false;
				// Handle disconnection
				handleClientDisconnect(client_sock);
				continue;
			}

			std::string newRequest(buf.begin(), buf.end());
			Action emptyAction;


			Action reqDetail = deconstructReq(newRequest);
			switch (reqDetail.code)
			{
			case MC_INSERT_REQUEST:
				repCode = std::to_string(MC_INSERT_RESP);
				break;

			case MC_DELETE_REQUEST:
				repCode = std::to_string(MC_DELETE_RESP);
				break;

			case MC_REPLACE_REQUEST:
				repCode = std::to_string(MC_REPLACE_RESP);
				break;

			case MC_LOGIN_REQUEST:
				send = false;
				for (auto it = m_clients.begin(); it != m_clients.end(); ++it)
				{
					if (it->second->getUsername() == reqDetail.userName || it->second->getEmail() == reqDetail.email)
					{
						throw std::exception("User already logged in");
						send = true;  // Indicate that the response has been sent
						break;  // Exit the loop
					}
				}

				// If the response has been sent, don't proceed to the second condition
				if (!send)
				{
					send = false;
					if (m_database->doesUserExist(reqDetail.userName) && m_database->doesPasswordMatch(reqDetail.userName, reqDetail.pass))
					{
						repCode = std::to_string(MC_LOGIN_RESP);
						reqDetail.userName = m_database->getUserName(reqDetail.email);
						reqDetail.email = m_database->getEmail(reqDetail.userName);
						Client* client_handler = new Client(m_database->getUserId(reqDetail.userName), reqDetail.userName, reqDetail.email);
						m_clients[client_sock] = client_handler;
						initialFileContent = repCode + std::to_string(client_handler->getId());
						Helper::sendData(client_sock, BUFFER(initialFileContent.begin(), initialFileContent.end()));
					}
					else
					{
						throw std::exception("invalid username or password.");
					}
				}
				break;

			case MC_SIGNUP_REQUEST:
				send = false;
				if (!m_database->doesUserExist(reqDetail.userName) && !m_database->doesUserExist(reqDetail.email))
				{
					m_database->addNewUser(reqDetail.userName, reqDetail.pass, reqDetail.email);
					repCode = std::to_string(MC_SIGNUP_RESP);
					Client* client_handler = new Client(m_database->getUserId(reqDetail.userName), reqDetail.userName, reqDetail.email);
					m_clients[client_sock] = client_handler;
					initialFileContent = repCode + std::to_string(client_handler->getId());
					Helper::sendData(client_sock, BUFFER(initialFileContent.begin(), initialFileContent.end()));
				}
				else
				{
					throw std::exception("Invalid name or email");
				}
				break;

			case MC_FORGOT_PASSW_REQUEST:
				/*
				// Retrieve the email associated with the username from the database
				std::string userEmail = m_database->getEmailFromDatabase(reqDetail.userName);

				if (userEmail != "")
				{
					// Generate a password reset token or a new password
					std::string resetToken = generateResetToken(); // Implement this function

					// Send a password reset email
					sendPasswordResetEmail(userEmail, resetToken); // Implement this function

					// Send a response to the client indicating success
					repCode = std::to_string(MC_FORGOT_PASSW_RESP);
					Helper::sendData(client_sock, BUFFER(repCode.begin(), repCode.end()));
				}
				else
				{
					// Send a response to the client indicating failure (username not found)
					throw std::exception("Username not found");
				}
				*/

				break;

			case MC_INITIAL_REQUEST:
				repCode = std::to_string(MC_INITIAL_RESP);
				fileContent = fileOperationHandler.readFromFile(".\\files\\" + reqDetail.data);
				// Convert the length to a string with exactly 5 digits
				lengthString = std::to_string(fileContent.length());
				lengthString = std::string(5 - lengthString.length(), '0') + lengthString;

				emptyAction.code = MC_INITIAL_REQUEST;
				m_lastActionMap[".\\files\\" + reqDetail.data] = emptyAction;

				// Create the initialFileContent string
				initialFileContent = repCode + lengthString + fileContent;
				Helper::sendData(client_sock, BUFFER(initialFileContent.begin(), initialFileContent.end()));
				send = false;
				break;
			case MC_CREATE_FILE_REQUEST:
				// Check if the file with the specified name exists
				if (fileOperationHandler.fileExists(".\\files\\" + reqDetail.data + ".txt"))
				{
					// File already exists, send an appropriate response code
					throw std::exception("file already exists");
					send = false;
				}
				else
				{
					// File doesn't exist, create it and send a success response code
					repCode = std::to_string(MC_APPROVE_RESP);
					Helper::sendData(client_sock, BUFFER(repCode.begin(), repCode.end()));

					// Create the mutex for the new file
					m_fileMutexes[".\\files\\" + reqDetail.data + ".txt"];
					fileOperationHandler.createFile(".\\files\\" + reqDetail.data + ".txt", true);
					m_database->createChat(reqDetail.data);
					m_files.push_back(reqDetail.data + ".txt");

					repCode = std::to_string(MC_ADD_FILE_RESP) + reqDetail.data + ".txt";
					m_clients[client_sock]->setFileName(".\\files\\" + reqDetail.data + ".txt");

					notifyAllClients(repCode, client_sock, false);

					send = false;

					emptyAction.code = MC_CREATE_FILE_REQUEST;
					m_lastActionMap[".\\files\\" + reqDetail.data + ".txt"] = emptyAction;
					m_usersOnFile[".\\files\\" + reqDetail.data + ".txt"].push_back(*m_clients[client_sock]);
				}
				break;

			case MC_DELETE_FILE_REQUEST:
				// Check if someone is using the file
				if (m_usersOnFile.find(".\\files\\" + reqDetail.data + ".txt") != m_usersOnFile.end() 
					&& !m_usersOnFile[".\\files\\" + reqDetail.data + ".txt"].empty())
				{
					throw std::exception("cannot delete. Someone is inside");
				}
				else
				{
					repCode = std::to_string(MC_DELETE_FILE_RESP) + reqDetail.data + ".txt";
					m_files.erase(std::find(m_files.begin(), m_files.end(), reqDetail.data + ".txt"));
					fileOperationHandler.deleteFile(".\\files\\" + reqDetail.data + ".txt");
					m_database->DeleteChat(reqDetail.data);
					Helper::sendData(client_sock, BUFFER(repCode.begin(), repCode.end()));
					notifyAllClients(repCode, client_sock, false);
				}
				send = false;
				break;

			case MC_GET_FILES_REQUEST:
				repCode = std::to_string(MC_GET_FILES_RESP);
				send = false;
				fileOperationHandler.getFilesInDirectory(".\\files", m_files);
				for (const auto& fileName : m_files)
				{
					lengthString = std::to_string(fileName.length());
					lengthString = std::string(5 - lengthString.length(), '0') + lengthString;
					repCode += lengthString + fileName;
				}
				Helper::sendData(client_sock, BUFFER(repCode.begin(), repCode.end()));
				break;
			case MC_GET_MESSAGES_REQUEST:
				send = false;
				// Handle get messages request
				repCode = std::to_string(MC_GET_MESSAGES_RESP);
				repCode += m_database->GetChatData(reqDetail.data);
				Helper::sendData(client_sock, BUFFER(repCode.begin(), repCode.end()));
				break;

			case MC_GET_USERS_REQUEST:
				// Handle get users request
				repCode = std::to_string(MC_GET_USERS_RESP);
				send = false;
				// Get the list of users logged into the file
				for (const auto& user : m_usersOnFile[".\\files\\" + reqDetail.data]) {
					lengthString = std::to_string((user.getUsername().length()));
					lengthString = std::string(5 - lengthString.length(), '0') + lengthString;
					repCode += lengthString + user.getUsername();
				}
				Helper::sendData(client_sock, BUFFER(repCode.begin(), repCode.end()));
				break;

			case MC_POST_MSG_REQUEST:
				// Handle post message request

				lengthString = std::to_string(m_clients[client_sock]->getUsername().length());
				lengthString = std::string(5 - lengthString.length(), '0') + lengthString;

				initialFileContent = reqDetail.dataLength + reqDetail.data +
					lengthString + m_clients[client_sock]->getUsername();
				m_database->UpdateChat(reqDetail.fileName, initialFileContent);

				repCode = std::to_string(MC_POST_MSG_RESP) + initialFileContent;
				notifyAllClients(repCode, client_sock, true);
				send = false;
				break;

			case MC_JOIN_FILE_REQUEST:
				repCode = std::to_string(MC_APPROVE_RESP);
				Helper::sendData(client_sock, BUFFER(repCode.begin(), repCode.end()));

				repCode = std::to_string(MC_JOIN_FILE_RESP);
				m_clients[client_sock]->setFileName(".\\files\\" + reqDetail.data);

				// Create the mutex for the file if it doesn't exist
				m_fileMutexes.try_emplace(".\\files\\" + reqDetail.data);

				m_usersOnFile[".\\files\\" + reqDetail.data].push_back(*m_clients[client_sock]);

				lengthString = std::to_string((m_clients[client_sock]->getUsername().length()));
				lengthString = std::string(5 - lengthString.length(), '0') + lengthString;
				repCode += lengthString + m_clients[client_sock]->getUsername();

				notifyAllClients(repCode, client_sock, true);
				send = false;
				break;
			case MC_LEAVE_FILE_REQUEST:
				repCode = std::to_string(MC_APPROVE_RESP);
				Helper::sendData(client_sock, BUFFER(repCode.begin(), repCode.end()));

				repCode = std::to_string(MC_LEAVE_FILE_RESP);
				reqDetail.fileName = m_clients[client_sock]->getFileName();

				for (auto it = m_usersOnFile.begin(); it != m_usersOnFile.end(); ++it) {
					// Iterate over the array of clients for each file
					for (auto clientIt = it->second.begin(); clientIt != it->second.end(); ) {
						if (clientIt->getId() == m_clients[client_sock]->getId()) {
							clientIt = it->second.erase(clientIt);
						}
						else {
							++clientIt;
						}
					}
				}

				lengthString = std::to_string((m_clients[client_sock]->getUsername().length()));
				lengthString = std::string(5 - lengthString.length(), '0') + lengthString;
				repCode += lengthString + m_clients[client_sock]->getUsername();
				notifyAllClients(repCode, client_sock, true);
				send = false;

				// Check if the user leaving was the last one
				if (m_usersOnFile[reqDetail.fileName].empty()) {
					// Delete the mutex and remove the file from m_usersOnFile
					m_fileMutexes.erase(reqDetail.fileName);
					m_usersOnFile.erase(reqDetail.fileName);
					m_lastActionMap.erase(reqDetail.fileName);
				}
				m_clients[client_sock]->setFileName("");
				break;
			case MC_DISCONNECT: // Handle disconnect request
				run = false;
				handleClientDisconnect(client_sock);
				send = false;
				continue;
			default:
				// Handle the default case or throw an error
				throw std::runtime_error("Unknown action code: " + reqDetail.msg);
			}

			if (send)
			{
				{
					std::string fileName = m_clients[client_sock]->getFileName();
					// Lock the mutex before updating the file
					std::lock_guard<std::mutex> lock(m_fileMutexes[fileName]);

					reqDetail = adjustIndexForSync(fileName, reqDetail);
					reqDetail.fileName = fileName;
					updateFileOnServer(fileName, reqDetail);
					notifyAllClients(repCode + reqDetail.msg, client_sock, true);

					reqDetail.timestamp = getCurrentTimestamp();
					m_lastActionMap[fileName] = reqDetail;
				}// lock goes out of scope, releasing the lock

			}
			send = true;


		}
		catch (const std::exception& e)
		{
			// Check if it's a connection error
			if (Helper::IsConnectionError(e))
			{
				run = false;
				// Handle connection error
				handleClientDisconnect(client_sock);
			}
			else
			{
				// Handle other exceptions
				std::string initialFileContent = std::to_string(MC_ERR_RESP) + e.what();
				Helper::sendData(client_sock, BUFFER(initialFileContent.begin(), initialFileContent.end()));
			}
		}

	}

	closesocket(client_sock);
}

Action Communicator::adjustIndexForSync(const std::string& fileName, Action reqDetail)
{
	std::string lengthString;
	std::string selectionLengthString;
	std::string indexString;

	int selectionLength;
	int length;
	std::string data;
	int newIndex;

	int newCode = reqDetail.code;
	// Check if there is a last action recorded for this file
	if (m_lastActionMap.find(fileName) != m_lastActionMap.end())
	{
		int lastUser = m_lastActionMap[fileName].userId;
		long long lastTimestamp = m_lastActionMap[fileName].timestamp;
		if (m_lastActionMap[fileName].code != MC_INITIAL_REQUEST && 
			m_lastActionMap[fileName].code != MC_CREATE_FILE_REQUEST &&
			lastUser != reqDetail.userId && 
			lastTimestamp > reqDetail.timestamp)
		{
			int lastActionCode = m_lastActionMap[fileName].code;
			int size = m_lastActionMap[fileName].size;
			int lastIndex = std::stoi(m_lastActionMap[fileName].index);

			std::string newAction = reqDetail.msg;

			std::string adjustedIndex = reqDetail.index;
			std::string updatedAction = newAction;

			newIndex = std::stoi(reqDetail.index);

			//reqDetail.timestamp = getCurrentTimestamp();

			// uodate the index
			switch (lastActionCode) {
			case MC_INSERT_REQUEST:
				if (newIndex > lastIndex)
				{
					newIndex += size;
					adjustedIndex = std::to_string(newIndex);
					adjustedIndex = std::string(5 - adjustedIndex.length(), '0') + adjustedIndex;
					updatedAction = reqDetail.dataLength + reqDetail.data + adjustedIndex;
				}
				break;
			case MC_DELETE_REQUEST:
				if (newIndex > lastIndex)
				{
					newIndex -= size;
					adjustedIndex = std::to_string(newIndex);
					adjustedIndex = std::string(5 - adjustedIndex.length(), '0') + adjustedIndex;
					updatedAction = reqDetail.dataLength + adjustedIndex;
				}
				break;
			case MC_REPLACE_REQUEST:
				if (newIndex > lastIndex)
				{
					newIndex = newIndex - std::stoi(reqDetail.selectionLength) + std::stoi(reqDetail.dataLength) ;
					adjustedIndex = std::to_string(newIndex);
					adjustedIndex = std::string(5 - adjustedIndex.length(), '0') + adjustedIndex;
					updatedAction = reqDetail.selectionLength + reqDetail.dataLength + reqDetail.data + adjustedIndex;
				}
				break;
			}
			reqDetail.index = adjustedIndex;
			reqDetail.msg = updatedAction;
		}
	}
	return reqDetail;

}

void Communicator::handleClientDisconnect(SOCKET client_sock)
{
	// Check if the client is associated with a file
	if (m_clients.find(client_sock) != m_clients.end())
	{
		Client* disconnectedClient = m_clients[client_sock];

		// Check if the client is inside a file
		if (disconnectedClient->getFileName() != "")
		{
			std::string fileName = disconnectedClient->getFileName();

			// Remove the client from the file's user list
			auto it = m_usersOnFile.find(fileName);
			if (it != m_usersOnFile.end())
			{
				it->second.erase(std::remove_if(it->second.begin(), it->second.end(),
					[disconnectedClient](const Client& client) {
						return client.getId() == disconnectedClient->getId();
					}), it->second.end());

				if (!it->second.empty()) {
					// Notify other clients in the same file about the disconnection
					std::string lengthString = std::to_string(disconnectedClient->getUsername().length());
					lengthString = std::string(5 - lengthString.length(), '0') + lengthString;
					std::string repCode = std::to_string(MC_LEAVE_FILE_RESP) + lengthString + disconnectedClient->getUsername();

					// Notify all clients in the same file
					notifyAllClients(repCode, client_sock, true);
				}

				if (m_usersOnFile[fileName].empty()) {
					// Delete the mutex and remove the file from m_usersOnFile
					m_fileMutexes.erase(fileName);
					m_usersOnFile.erase(fileName);
					m_lastActionMap.erase(fileName);
				}
			}
		}

		// Clean up resources and remove the client from the map
		delete disconnectedClient;
		m_clients.erase(client_sock);
	}
}

Action Communicator::deconstructReq(const std::string& req) {
	std::string msgCode = req.substr(0, 3);
	std::string action = req.substr(3);

	Action newAction;    
	std::string indexString;

	switch (std::stoi(msgCode))
	{
	case MC_INITIAL_REQUEST:
		newAction.data = action;
		break;
	case MC_INSERT_REQUEST:
		newAction.dataLength = action.substr(0, 5);
		newAction.size = std::stoi(newAction.dataLength);

		newAction.data = action.substr(5, newAction.size);
		newAction.index = action.substr(5 + newAction.size);
		break;

	case MC_DELETE_REQUEST:
		newAction.dataLength = action.substr(0, 5);
		indexString = action.substr(5);

		newAction.size = std::stoi(newAction.dataLength);
		newAction.index = indexString;
		break;

	case MC_REPLACE_REQUEST:
		newAction.selectionLength = action.substr(0, 5);
		newAction.dataLength = action.substr(5, 5);
		newAction.size = std::stoi(newAction.dataLength);
		newAction.data = action.substr(10, newAction.size);
		indexString = action.substr(10 + newAction.size);
		newAction.index = indexString;
		break;
	case MC_CREATE_FILE_REQUEST:
		newAction.data = action;
		break;
	case MC_GET_FILES_REQUEST:
		//newAction.data = action;
		break;
	case MC_GET_MESSAGES_REQUEST:
		newAction.data = action;
		break;
	case MC_GET_USERS_REQUEST:
		newAction.data = action;
		break;
	case MC_POST_MSG_REQUEST:
		newAction.fileNameLength = action.substr(0, 5);
		newAction.size = std::stoi(newAction.fileNameLength);
		newAction.fileName = action.substr(5, newAction.size);
		newAction.dataLength = action.substr(5 + newAction.size, 5);
		newAction.data = action.substr(10 + newAction.size, std::stoi(newAction.dataLength));
		break;
	case MC_JOIN_FILE_REQUEST:
		newAction.dataLength = action.substr(0, 5);
		newAction.data = action.substr(5, std::stoi(newAction.dataLength));
		break;
	case MC_LEAVE_FILE_REQUEST:
		newAction.data = action.substr(0, 5);
		break;
	case MC_DELETE_FILE_REQUEST:
		newAction.dataLength = action.substr(0, 5);
		newAction.data = action.substr(5, std::stoi(newAction.dataLength));
		break;
	case MC_LOGIN_REQUEST:
		newAction.userNameLength = std::stoi(action.substr(0,5));
		newAction.userName = action.substr(5, newAction.userNameLength);
		newAction.email = newAction.userName;

		newAction.passLength = std::stoi(action.substr(5 + newAction.userNameLength, 5));
		newAction.pass = action.substr(10 + newAction.userNameLength, newAction.passLength);
		break;
	case MC_SIGNUP_REQUEST:
		newAction.userNameLength = std::stoi(action.substr(0, 5));
		newAction.userName = action.substr(5, newAction.userNameLength);

		newAction.passLength = std::stoi(action.substr(5 + newAction.userNameLength, 5));
		newAction.pass = action.substr(10 + newAction.userNameLength, newAction.passLength);

		newAction.emailLength = std::stoi(action.substr(10 + newAction.userNameLength + newAction.passLength, 5));
		newAction.email = action.substr(15 + newAction.userNameLength + newAction.passLength, newAction.emailLength);
		break;
	case MC_FORGOT_PASSW_REQUEST:
		newAction.userNameLength = std::stoi(action.substr(0, 5));
		newAction.userName = action.substr(5, newAction.userNameLength);
		break;
	}
	newAction.timestamp = getCurrentTimestamp();
	newAction.code = std::stoi(msgCode);
	newAction.msg = action;
	return newAction;
}

void Communicator::updateFileOnServer(const std::string& filePath, const Action& reqDetail)
{
	std::fstream file(filePath, std::ios::in | std::ios::out);
	if (!file.is_open()) {
		throw std::runtime_error("Failed to open file for reading/writing: " + filePath);
	}
	else {
		switch (reqDetail.code) {
		case MC_INSERT_REQUEST:
			// Insert operation
			operationHandler.insert(file, reqDetail.data, std::stoi(reqDetail.index));
			break;

		case MC_DELETE_REQUEST:
			// Delete operation
			operationHandler.deleteContent(file, std::stoi(reqDetail.dataLength), std::stoi(reqDetail.index), reqDetail.fileName);
			break;

		case MC_REPLACE_REQUEST:
			// Replace operation
			operationHandler.replace(file, std::stoi(reqDetail.selectionLength), reqDetail.data, std::stoi(reqDetail.index));
			break;

		default:
			throw std::runtime_error("Unknown action code: " + reqDetail.code);
		}

		file.close();
	}
}

void Communicator::updateChatFileOnServer(const std::string& filePath, const Action& reqDetail)
{
	std::ofstream file(filePath, std::ios::app);  // Open file in append mode

	if (!file.is_open()) {
		throw std::runtime_error("Failed to open file for writing: " + filePath);
	}
	else {
		// Format the message
		std::string message = reqDetail.dataLength + reqDetail.data + reqDetail.index ;

		// Append the message to the file
		file << message;

		// Close the file
		file.close();
	}
}

void Communicator::notifyAllClients(const std::string& updatedContent, SOCKET client_sock, const bool isOnFile)
{
	// Iterate through all connected clients and send them the updated content
	for (auto& sock : m_clients)
	{
		if (sock.first != client_sock)
		{
			if (isOnFile && m_clients[client_sock]->getFileName() == m_clients[sock.first]->getFileName())
			{
				SOCKET client_sock = sock.first;
				Helper::sendData(client_sock, BUFFER(updatedContent.begin(), updatedContent.end()));
			}
			else if(!isOnFile && m_clients[sock.first]->getFileName() == "")
			{
				SOCKET client_sock = sock.first;
				Helper::sendData(client_sock, BUFFER(updatedContent.begin(), updatedContent.end()));
			}
		}
	}
}

long long Communicator::getCurrentTimestamp() {
	auto currentTime = std::chrono::system_clock::now();
	auto duration = currentTime.time_since_epoch();

	// Convert duration to milliseconds
	auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration);

	// Convert milliseconds to a long long value
	return milliseconds.count();
}

void Communicator::startHandleRequests()
{
	SOCKET client_socket;
	bindAndListen();
	while (true)
	{
		client_socket = accept(m_serverSocket, NULL, NULL);
		if (client_socket == INVALID_SOCKET)
			throw std::exception("Recieved an invalid socket.");
		std::thread t(&Communicator::handleNewClient, this, client_socket);
		t.detach();
	}
}