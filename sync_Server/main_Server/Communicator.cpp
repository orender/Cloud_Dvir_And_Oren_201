#include "Communicator.h"
#define CLOUDGOODCOMMAND 420
#define CLOUDBADCOMMAND 42


extern std::unordered_map<std::string, std::mutex> m_fileMutexes;

Communicator::Communicator()
{
	// this server use TCP. that why SOCK_STREAM & IPPROTO_TCP
	// if the server use UDP we will use: SOCK_DGRAM & IPPROTO_UDP
	m_serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	m_cloudServerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_serverSocket == INVALID_SOCKET)
		throw std::exception("Failed to initialize server socket.");
	if (m_cloudServerSocket == INVALID_SOCKET)
		throw std::exception("Failed to initialize cloud server socket.");

	//initialize socket
	m_cloudServerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	SOCKADDR_IN _addr;
	//set up info for connection
	_addr.sin_family = AF_INET;
	_addr.sin_port = htons(CLOUD_PORT);
	_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	CloudConnected = true;
	if (connect(m_cloudServerSocket, (SOCKADDR*)&_addr, sizeof(_addr)) == SOCKET_ERROR)
	{
		std::cout << ("Failed to connect to cloud server.\n");
		CloudConnected = false;
	}
}


// Destructor
Communicator::~Communicator() {
	try {
		closesocket(m_serverSocket);
		closesocket(m_cloudServerSocket);
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


void Communicator::login(std::string msg, SOCKET client_sock)
{
	Action reqDetail = deconstructReq(msg);

	bool pass = false;
	for (auto it = m_clients.begin(); it != m_clients.end(); ++it)
	{
		if (it->second->getUsername() == reqDetail.userName || it->second->getEmail() == reqDetail.email)
		{
			throw std::exception("User already logged in");
			pass = true;  // Indicate that the response has been sent
			break;  // Exit the loop
		}
	}

	// If the response has been sent, don't proceed to the second condition
	if (!pass)
	{
		if (m_database->doesUserExist(reqDetail.userName) && m_database->doesPasswordMatch(reqDetail.userName, reqDetail.pass))
		{
			std::string repCode = std::to_string(MC_LOGIN_RESP);
			reqDetail.userName = m_database->getUserName(reqDetail.email, -1);
			reqDetail.email = m_database->getEmail(reqDetail.userName);
			Client* client_handler = new Client(m_database->getUserId(reqDetail.userName), reqDetail.userName, reqDetail.email);
			m_clients[client_sock] = client_handler;

			std::string lengthString = std::to_string(reqDetail.userName.length());
			lengthString = std::string(5 - lengthString.length(), '0') + lengthString;
			repCode += lengthString + reqDetail.userName + std::to_string(client_handler->getId());

			Helper::sendData(client_sock, BUFFER(repCode.begin(), repCode.end()));

			repCode = std::to_string(MC_LOGIN_RESP) + reqDetail.userName;
			notifyAllClients(repCode, client_sock, false);
		}
		else
		{
			throw std::exception("invalid username or password.");
		}
	}
}

void Communicator::signUp(std::string msg, SOCKET client_sock)
{
	Action reqDetail = deconstructReq(msg);

	if (!m_database->doesUserExist(reqDetail.userName) && !m_database->doesUserExist(reqDetail.email))
	{
		m_database->addNewUser(reqDetail.userName, reqDetail.pass, reqDetail.email);
		std::string repCode = std::to_string(MC_SIGNUP_RESP);
		Client* client_handler = new Client(m_database->getUserId(reqDetail.userName), reqDetail.userName, reqDetail.email);
		m_clients[client_sock] = client_handler;
		std::string initialFileContent = repCode + std::to_string(client_handler->getId());
		Helper::sendData(client_sock, BUFFER(initialFileContent.begin(), initialFileContent.end()));

		repCode += reqDetail.userName;
		notifyAllClients(repCode, client_sock, false);
	}
	else
	{
		throw std::exception("Invalid name or email");
	}
}

void Communicator::createFile(std::string msg, SOCKET client_sock)
{
	char* buffer = new char[1024];
	char recvbuf[1024];
	size_t bufferSize;
	int code = 0;
	std::string msgFromCloud;

	Action reqDetail = deconstructReq(msg);

	// Check if the file with the specified name exists
	//if (fileOperationHandler.fileExists(".\\files\\" + reqDetail.data + ".txt"))
	if (m_database->getFileDetails(reqDetail.data + ".txt").fileName != "")
	{
		// File already exists, send an appropriate response code
		throw std::exception("file already exists");
	}
	else
	{
		// File doesn't exist, create it and send a success response code
		std::string repCode = std::to_string(MC_APPROVE_REQ_RESP);
		Helper::sendData(client_sock, BUFFER(repCode.begin(), repCode.end()));

		//TODO check that deleted succefully
		code = CLOUDGOODCOMMAND;
		if (CloudConnected)
		{
			writeMessage(5, reqDetail.data + ".txt", buffer, bufferSize);
			send(m_cloudServerSocket, buffer, bufferSize, 0);

			recv(m_cloudServerSocket, recvbuf, 1024, 0);
			code = 0;
			msgFromCloud = "";
			readMessage(recvbuf, code, msgFromCloud);
		}
		else
		{
			fileOperationHandler.createFile(".\\files\\" + reqDetail.data + ".txt", true); // decide if needs to be removed later
		}

		std::cout << msgFromCloud << std::endl;
		if (code == CLOUDGOODCOMMAND)
		{
			Action emptyAction;
			// Create the mutex for the new file
			m_fileMutexes[".\\files\\" + reqDetail.data + ".txt"];
			m_database->createChat(reqDetail.data);
			m_database->addFile(m_clients[client_sock]->getId(), reqDetail.data + ".txt");

			FileDetail fileList = m_database->getFileDetails(reqDetail.data + ".txt");
			m_fileNames[reqDetail.data + ".txt"] = fileList.fileId;

			m_database->addUserPermission(m_clients[client_sock]->getId(), m_fileNames[reqDetail.data + ".txt"]);

			repCode = std::to_string(MC_ADD_FILE_RESP) + reqDetail.data + ".txt";
			m_clients[client_sock]->setFileName(".\\files\\" + reqDetail.data + ".txt");

			notifyAllClients(repCode, client_sock, false);

			emptyAction.code = MC_CREATE_FILE_REQUEST;
			m_lastActionMap[".\\files\\" + reqDetail.data + ".txt"].push_back(emptyAction);
			m_usersOnFile[".\\files\\" + reqDetail.data + ".txt"].push_back(*m_clients[client_sock]);

			repCode = std::to_string(MC_JOIN_FILE_RESP);

			std::string lengthString = std::to_string((m_clients[client_sock]->getUsername().length()));
			lengthString = std::string(5 - lengthString.length(), '0') + lengthString;
			repCode += lengthString + m_clients[client_sock]->getUsername();

			reqDetail.data += ".txt";
			lengthString = std::to_string((reqDetail.data.length()));
			lengthString = std::string(5 - lengthString.length(), '0') + lengthString;
			repCode += lengthString + reqDetail.data;

			notifyAllClients(repCode, client_sock, false);
		}
		else {
			throw std::exception(msgFromCloud.c_str());
		}

	}
}

void Communicator::deleteFile(std::string msg, SOCKET client_sock)
{
	char* buffer = new char[1024];
	char recvbuf[1024];
	size_t bufferSize;
	int code = 0;
	std::string msgFromCloud;

	Action reqDetail = deconstructReq(msg);

	if (m_usersOnFile.find(".\\files\\" + reqDetail.data + ".txt") != m_usersOnFile.end()
		&& !m_usersOnFile[".\\files\\" + reqDetail.data + ".txt"].empty())
	{
		throw std::exception("cannot delete. Someone is inside");
	}
	else if (!m_database->hasPermission(m_clients[client_sock]->getId(), m_database->getFileDetails(reqDetail.data + ".txt").fileId))
	{
		throw std::exception("dont have permission for this file");
	}
	else
	{
		std::string repCode = std::to_string(MC_DELETE_FILE_RESP) + reqDetail.data + ".txt";

		//TODO check that deleted succefully
		if (CloudConnected)
		{
			writeMessage(3, reqDetail.data + ".txt", buffer, bufferSize);
			send(m_cloudServerSocket, buffer, bufferSize, 0);

			recv(m_cloudServerSocket, recvbuf, 1024, 0);
			code = 0;
			msgFromCloud = "";
			readMessage(recvbuf, code, msgFromCloud);
		}
		else
		{
			fileOperationHandler.deleteFile(".\\files\\" + reqDetail.data + ".txt"); // decide if needs to be removed later

		}
		if (code == CLOUDGOODCOMMAND)
		{
			m_database->DeleteChat(reqDetail.data);
			m_database->deleteFile(reqDetail.data + ".txt");
			m_database->deletePermission(m_fileNames[reqDetail.data + ".txt"]);
			m_database->deleteAllPermissionReq(m_fileNames[reqDetail.data + ".txt"]);
			m_fileNames.erase(reqDetail.data + ".txt");

			Helper::sendData(client_sock, BUFFER(repCode.begin(), repCode.end()));
			notifyAllClients(repCode, client_sock, false);
			Helper::sendData(client_sock, BUFFER(repCode.begin(), repCode.end()));
		}
		else {
			throw std::exception(msgFromCloud.c_str());
		}

	}
}

void Communicator::getFiles(std::string msg, SOCKET client_sock)
{
	std::string repCode = std::to_string(MC_GET_FILES_RESP);
	
	char* buffer = new char[1024];
	char recvbuf[1024];
	size_t bufferSize;
	int code = 0;
	std::string msgFromCloud;

	Action reqDetail = deconstructReq(msg);

	if (CloudConnected)
	{
		writeMessage(4, "", buffer, bufferSize); // send empty request
		send(m_cloudServerSocket, buffer, bufferSize, 0);

		recv(m_cloudServerSocket, recvbuf, 1024, 0);
		code = 0;
		msgFromCloud = "";
		readMessage(recvbuf, code, msgFromCloud); // message is in the same format as the fortmat that is sent to th user
		fileOperationHandler.addFiles(msgFromCloud, m_fileNames);
		std::cout << msgFromCloud << std::endl;
	}
	else
	{
		fileOperationHandler.getFilesInDirectory(".\\files", m_fileNames);
	}

	for (const auto& fileName : m_fileNames)
	{
		std::string lengthString = std::to_string(fileName.first.length());
		lengthString = std::string(5 - lengthString.length(), '0') + lengthString;
		repCode += lengthString + fileName.first;

		FileDetail fileList = m_database->getFileDetails(fileName.first);
		m_fileNames[fileName.first] = fileList.fileId;
	}
	Helper::sendData(client_sock, BUFFER(repCode.begin(), repCode.end()));
}

void Communicator::getInitialContent(std::string msg, SOCKET client_sock)
{
	std::string repCode = std::to_string(MC_GET_FILES_RESP);

	char* buffer = new char[1024];
	char recvbuf[1024];
	size_t bufferSize;
	int code = 0;
	std::string msgFromCloud;

	std::string fileContent;

	Action reqDetail = deconstructReq(msg);
	Action emptyAction;

	repCode = std::to_string(MC_INITIAL_RESP);

	if (CloudConnected)
	{
		/*if (!m_filesData[".\\files\\" + reqDetail.data].empty())
		{

			fileContent = m_filesData[".\\files\\" + reqDetail.data];
		}
		else
		{
			writeMessage(2, reqDetail.data, buffer, bufferSize);
			send(m_cloudServerSocket, buffer, bufferSize, 0);

			recv(m_cloudServerSocket, recvbuf, 1024, 0);
			code = 0;
			msg = "";
			readMessage(recvbuf, code, msg);
			m_filesData[".\\files\\" + reqDetail.data] = msg;
			std::cout << msg << std::endl;
			fileContent = msg;
			m_filesData[".\\files\\" + reqDetail.data] = msg;
		}*/
		writeMessage(2, reqDetail.data, buffer, bufferSize);
		send(m_cloudServerSocket, buffer, bufferSize, 0);

		recv(m_cloudServerSocket, recvbuf, 1024, 0);
		code = 0;
		msg = "";
		readMessage(recvbuf, code, msg);
		m_filesData[".\\files\\" + reqDetail.data] = msg;
		std::cout << msg << std::endl;
		fileContent = msg;
	}
	else
	{
		fileContent = fileOperationHandler.readFromFile(".\\files\\" + reqDetail.data);
		m_filesData[".\\files\\" + reqDetail.data] = fileContent;

	}

	//m_filesData[".\\files\\" + reqDetail.data] = fileContent; // delete when using the cloud communication

	m_FileUpdate[".\\files\\" + reqDetail.data] = false;
	// Convert the length to a string with exactly 5 digits
	std::string lengthString = std::to_string(fileContent.length());
	lengthString = std::string(5 - lengthString.length(), '0') + lengthString;

	emptyAction.code = MC_INITIAL_REQUEST;
	m_lastActionMap[".\\files\\" + reqDetail.data].push_back(emptyAction);

	// Create the initialFileContent string
	std::string initialFileContent = repCode + lengthString + fileContent;
	Helper::sendData(client_sock, BUFFER(initialFileContent.begin(), initialFileContent.end()));

}

void Communicator::joinFile(std::string msg, SOCKET client_sock)
{
	Action reqDetail = deconstructReq(msg);

	int fileId = m_database->getFileDetails(reqDetail.data).fileId;
	std::string repCode;

	if (!m_database->hasPermission(m_clients[client_sock]->getId(), m_database->getFileDetails(reqDetail.data).fileId)) {
		// Send an error response indicating lack of permission
		std::string errMsg = "You are not allowed to join this file" + reqDetail.dataLength + reqDetail.data;
		throw std::exception(errMsg.c_str());
	}

	std::string lengthString = std::to_string((reqDetail.data.length()));
	lengthString = std::string(5 - lengthString.length(), '0') + lengthString;
	repCode = std::to_string(MC_APPROVE_JOIN_RESP) + lengthString + reqDetail.data;
	Helper::sendData(client_sock, BUFFER(repCode.begin(), repCode.end()));

	repCode = std::to_string(MC_JOIN_FILE_RESP);
	m_clients[client_sock]->setFileName(".\\files\\" + reqDetail.data);

	// Create the mutex for the file if it doesn't exist
	m_fileMutexes.try_emplace(".\\files\\" + reqDetail.data);

	m_usersOnFile[".\\files\\" + reqDetail.data].push_back(*m_clients[client_sock]);

	lengthString = std::to_string((m_clients[client_sock]->getUsername().length()));
	lengthString = std::string(5 - lengthString.length(), '0') + lengthString;
	repCode += lengthString + m_clients[client_sock]->getUsername();

	lengthString = std::to_string((reqDetail.data.length()));
	lengthString = std::string(5 - lengthString.length(), '0') + lengthString;
	repCode += lengthString + reqDetail.data;

	notifyAllClients(repCode, client_sock, true);
	notifyAllClients(repCode, client_sock, false);
}

void Communicator::leaveFile(std::string msg, SOCKET client_sock)
{
	Action reqDetail = deconstructReq(msg);
	std::string repCode = std::to_string(MC_APPROVE_REQ_RESP);
	Helper::sendData(client_sock, BUFFER(repCode.begin(), repCode.end()));

	repCode = std::to_string(MC_LEAVE_FILE_RESP);
	std::string fileName = m_clients[client_sock]->getFileName();

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

	std::string lengthString = std::to_string((m_clients[client_sock]->getUsername().length()));
	lengthString = std::string(5 - lengthString.length(), '0') + lengthString;
	repCode += lengthString + m_clients[client_sock]->getUsername();

	lengthString = std::to_string((fileName.length()));
	lengthString = std::string(5 - lengthString.length(), '0') + lengthString;
	repCode += lengthString + fileName;

	notifyAllClients(repCode, client_sock, true);
	notifyAllClients(repCode, client_sock, false);

	// Check if the user leaving was the last one
	if (m_usersOnFile[fileName].empty()) {
		// Delete the mutex and remove the file from m_usersOnFile
		m_fileMutexes.erase(fileName);
		m_usersOnFile.erase(fileName);
		m_lastActionMap.erase(fileName);
	}
	m_clients[client_sock]->setFileName("");
}

void Communicator::handleNewClient(SOCKET client_sock)
{
	bool run = true;
	bool pass = true;
	std::string msg;
	BUFFER buf;
	BUFFER rep;
	
	std::string repCode;
	std::string fileContent;
	std::string lengthString;
	std::string initialFileContent;
	FileDetail fileList;
	
	char* bufferA = new char[1024];
	char recvbufA[1024];
	size_t bufferSizeA;
	int codeA = 0;
	std::string msgA;

	if (CloudConnected)
	{
		writeMessage(4, "", bufferA, bufferSizeA); // send empty request
		send(m_cloudServerSocket, bufferA, bufferSizeA, 0);

		recv(m_cloudServerSocket, recvbufA, 1024, 0);
		codeA = 0;
		msgA = "";
		readMessage(recvbufA, codeA, msgA); // message is in the same format as the fortmat that is sent to th user
		fileOperationHandler.addFiles(msg, m_fileNames);
	}
	else
	{
		fileOperationHandler.getFilesInDirectory(".\\files", m_fileNames);
	}

	for (const auto& fileName : m_fileNames)
	{
		fileList = m_database->getFileDetails(fileName.first);
		m_fileNames[fileName.first] = fileList.fileId;
	}

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
			
			char* buffer = new char[1024];
			char recvbuf[1024];
			size_t bufferSize;
			int code = 0;
			std::string msg;

			Action reqDetail = deconstructReq(newRequest);
			int msgCode = std::stoi(newRequest.substr(0, 3));
			switch (msgCode)
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
				login(newRequest, client_sock);
				pass = false;
				break;

			case MC_SIGNUP_REQUEST:
				pass = false;
				signUp(newRequest, client_sock);
				break;

			case MC_FORGOT_PASSW_REQUEST:
				break;

			case MC_INITIAL_REQUEST: // get data from cloud
				getInitialContent(newRequest, client_sock);
				pass = false;
				break;

			case MC_CREATE_FILE_REQUEST:
				pass = false;
				createFile(newRequest, client_sock);
				break;

			case MC_DELETE_FILE_REQUEST:
				deleteFile(newRequest, client_sock);
				pass = false;
				break;

			case MC_GET_FILES_REQUEST:
				pass = false;
				getFiles(newRequest, client_sock);
				break;

			case MC_GET_MESSAGES_REQUEST:
				pass = false;
				// Handle get messages request
				repCode = std::to_string(MC_GET_MESSAGES_RESP);
				repCode += m_database->GetChatData(reqDetail.data);
				Helper::sendData(client_sock, BUFFER(repCode.begin(), repCode.end()));
				break;

			case MC_GET_USERS_ON_FILE_REQUEST:
				// Handle get users request
				repCode = std::to_string(MC_GET_USERS_ON_FILE_RESP);
				pass = false;
				// Get the list of users logged into the file
				for (const auto& user : m_usersOnFile[".\\files\\" + reqDetail.data]) {
					lengthString = std::to_string((user.getUsername().length()));
					lengthString = std::string(5 - lengthString.length(), '0') + lengthString;
					repCode += lengthString + user.getUsername();
				}
				Helper::sendData(client_sock, BUFFER(repCode.begin(), repCode.end()));
				break;

			case MC_GET_USERS_REQUEST:
				// Handle get users request
				repCode = std::to_string(MC_GET_USERS_RESP);
				pass = false;

				for (auto& sock : m_clients)
				{
					lengthString = std::to_string(m_clients[sock.first]->getUsername().length());
					lengthString = std::string(5 - lengthString.length(), '0') + lengthString;
					repCode += lengthString + m_clients[sock.first]->getUsername();

					// Add file name length and file name to the response
					lengthString = std::to_string(m_clients[sock.first]->getFileName().length());
					lengthString = std::string(5 - lengthString.length(), '0') + lengthString;
					repCode += lengthString + m_clients[sock.first]->getFileName();
				}
				Helper::sendData(client_sock, BUFFER(repCode.begin(), repCode.end()));
				break;
			case MC_GET_USERS_PERMISSIONS_REQ_REQUEST:
				repCode = std::to_string(MC_APPROVE_REQ_RESP);
				Helper::sendData(client_sock, BUFFER(repCode.begin(), repCode.end()));

				repCode = std::to_string(MC_GET_USERS_PERMISSIONS_REQ_RESP);
				pass = false;

				for (auto& req : m_database->getPermissionRequests(m_clients[client_sock]->getId()))
				{
					lengthString = std::to_string(m_database->getUserName("", req.userId).length());
					lengthString = std::string(5 - lengthString.length(), '0') + lengthString;
					repCode += lengthString + m_database->getUserName("", req.userId);

					// Add file name length and file name to the response
					lengthString = std::to_string(m_database->getFileName(req.fileId).length());
					lengthString = std::string(5 - lengthString.length(), '0') + lengthString;
					repCode += lengthString + m_database->getFileName(req.fileId);
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
				pass = false;
				break;
			case MC_APPROVE_PERMISSION_REQUEST:
				repCode = std::to_string(MC_APPROVE_PERMISSION_RESP);
				m_database->deletePermissionRequests(m_database->getUserId(reqDetail.userName), m_database->getFileDetails(reqDetail.fileName).fileId);
				m_database->addUserPermission(m_database->getUserId(reqDetail.userName), m_database->getFileDetails(reqDetail.fileName).fileId);
				Helper::sendData(client_sock, BUFFER(repCode.begin(), repCode.end()));
				pass = false;
				break;
			case MC_REJECT_PERMISSION_REQUEST:
				repCode = std::to_string(MC_REJECT_PERMISSION_RESP);
				m_database->deletePermissionRequests(m_database->getUserId(reqDetail.userName), m_database->getFileDetails(reqDetail.fileName).fileId);
				Helper::sendData(client_sock, BUFFER(repCode.begin(), repCode.end()));
				pass = false;
				break;
			case MC_PERMISSION_FILE_REQ_REQUEST:
				fileList = m_database->getFileDetails(reqDetail.fileName);
				if (!m_database->doesPermissionRequestExist(m_database->getUserId(reqDetail.userName), fileList.fileId, fileList.creatorId))
				{
					repCode = std::to_string(MC_PERMISSION_FILE_REQ_RESP);
					m_database->addPermissionRequest(m_database->getUserId(reqDetail.userName), fileList.fileId, fileList.creatorId);
					repCode += reqDetail.fileNameLength + reqDetail.fileName;
				}
				else
				{
					repCode = std::to_string(MC_ERROR_RESP) + "Request already exist, waiting for the owner of the file to approve";
				}
				Helper::sendData(client_sock, BUFFER(repCode.begin(), repCode.end()));
				pass = false;
				break;
			case MC_JOIN_FILE_REQUEST:
				pass = false;
				joinFile(newRequest, client_sock);
				break;
			case MC_LEAVE_FILE_REQUEST:
				pass = false;
				leaveFile(newRequest, client_sock);
				break;
			case MC_DISCONNECT: // Handle disconnect request
				run = false;
				handleClientDisconnect(client_sock);
				pass = false;
				continue;
			default:
				// Handle the default case or throw an error
				throw std::runtime_error("Unknown action code: " + reqDetail.msg);
			}

			if (pass)
			{
				{
					std::string fileName = m_clients[client_sock]->getFileName();
					// Lock the mutex before updating the file
					std::lock_guard<std::mutex> lock(m_fileMutexes[fileName]);

					reqDetail = adjustIndexForSync(fileName, reqDetail);
					reqDetail.fileName = fileName;

					if (CloudConnected)
					{
						updateFileOnServer(fileName, reqDetail);
					}
					else
					{
						updateFileOnServerOld(fileName, reqDetail);
					}
					notifyAllClients(repCode + reqDetail.msg, client_sock, true);

					reqDetail.timestamp = getCurrentTimestamp();
					m_lastActionMap[fileName].push_back(reqDetail);
					m_FileUpdate[fileName] = true;
					CloudUpdate = true;
				}// lock goes out of scope, releasing the lock

			}
			pass = true;


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
				handleError(client_sock, e);
			}
		}

	}

	closesocket(client_sock);
}

void Communicator::cloudCommunicationFunction(/* Parameters for communication */) {
	try
	{
		while (CloudConnected) {
			if (CloudUpdate)
			{
				for (auto& [fileName, updated] : m_FileUpdate) {
					if (updated) {
						{
							std::lock_guard<std::mutex> lock(m_fileMutexes[fileName]);
							char* buffer = new char[1024];
							size_t bufferSize;
							std::string lengthString = std::to_string((fileName.length() - 8));
							lengthString = std::string(3 - lengthString.length(), '0') + lengthString;
							std::string repCode = lengthString + fileName.substr(8);

							writeMessage(1, repCode + m_filesData[fileName], buffer, bufferSize);
							send(m_cloudServerSocket, buffer, bufferSize, 0);

							char recvbuf[1024];
							recv(m_cloudServerSocket, recvbuf, 1024, 0);
							int code = 0;
							std::string msg = "";
							readMessage(recvbuf, code, msg);

							std::cout << code << ", " << msg << std::endl;
						}
						if (m_usersOnFile[fileName].empty()) {
							m_filesData.erase(fileName);
							m_FileUpdate.erase(fileName);
							CloudUpdate = false;
							if (m_FileUpdate.empty())
							{
								break;
							}
						}
						else
						{
							m_FileUpdate[fileName] = false;  // Reset the change flag
						}
					}
					else {
						if (m_usersOnFile[fileName].empty()) {
							m_filesData.erase(fileName);
							m_FileUpdate.erase(fileName);
							CloudUpdate = false;
						}
						if (m_FileUpdate.empty())
						{
							break;
						}
					}
				}
			}
			std::this_thread::sleep_for(std::chrono::seconds(20));
		}
	}
	catch (const std::exception& e)
	{

	}
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
		std::vector<Action>& lastActions = m_lastActionMap[fileName];

		// Use an iterator to iterate over the vector
		auto it = lastActions.begin();

		// Iterate over all last actions for the file
		while (it != lastActions.end())
		{
			const Action& action = *it;

			// Check if the new action was created before the current last action and by a different user
			if (reqDetail.timestamp < action.timestamp && reqDetail.userId != action.userId
				&& action.code != MC_INITIAL_REQUEST && action.code != MC_CREATE_FILE_REQUEST)
			{
				int lastActionCode = action.code;
				int size = action.size;
				int lastIndex = std::stoi(action.index);

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
						newIndex = newIndex - std::stoi(reqDetail.selectionLength) + std::stoi(reqDetail.dataLength);
						adjustedIndex = std::to_string(newIndex);
						adjustedIndex = std::string(5 - adjustedIndex.length(), '0') + adjustedIndex;
						updatedAction = reqDetail.selectionLength + reqDetail.dataLength + reqDetail.data + adjustedIndex;
					}
					break;
				}
				reqDetail.index = adjustedIndex;
				reqDetail.msg = updatedAction;
			}
			else if(reqDetail.timestamp > action.timestamp + 5)
			{
				it = lastActions.erase(it);
			}
			if (!lastActions.empty())
			{
				++it;
			}
		}
	}
	return reqDetail;

}

void Communicator::handleError(SOCKET client_sock, std::exception a)
{
	try
	{
		// Check if the client is associated with a file
		if (m_clients.find(client_sock) != m_clients.end())
		{
			Client* client = m_clients[client_sock];

			// Check if the client is currently working on a file
			if (!client->getFileName().empty())
			{
				// Notify the client about the error
				std::string response = std::to_string(MC_ERROR_RESP);
				
				std::string fileContent = fileOperationHandler.readFromFile(".\\files\\" + client->getFileName());
				std::string lengthString = std::to_string(fileContent.length());
				lengthString = std::string(5 - lengthString.length(), '0') + lengthString;

				response += lengthString + fileContent;

				Helper::sendData(client_sock, BUFFER(response.begin(), response.end()));

				// Send the latest version of the file to the client
				

				/*
				// If necessary, adjust and commit the client's request
				reqDetail = adjustIndexForSync(fileName, reqDetail);
				reqDetail.fileName = fileName;
				updateFileOnServer(fileName, reqDetail);

				// Notify all clients about the adjusted request
				std::string repCode = std::to_string(MC_ERR_ADJUSTED_RESP);
				notifyAllClients(repCode + reqDetail.msg, client_sock, true);

				// Update the last action map
				reqDetail.timestamp = getCurrentTimestamp();
				m_lastActionMap[fileName].push_back(reqDetail);
				*/
			}
			else
			{
				std::string initialFileContent = std::to_string(MC_ERROR_RESP) + a.what();
				Helper::sendData(client_sock, BUFFER(initialFileContent.begin(), initialFileContent.end()));
			}
		}
		else
		{
			std::string initialFileContent = std::to_string(MC_ERROR_RESP) + a.what();
			Helper::sendData(client_sock, BUFFER(initialFileContent.begin(), initialFileContent.end()));
		}
	}
	catch (const std::exception& e)
	{
		// Handle any further exceptions if needed
		// You can log the exception or take appropriate actions
		// ...
	}
}

void Communicator::handleClientDisconnect(SOCKET client_sock)
{
	// Check if the client is associated with a file
	if (m_clients.find(client_sock) != m_clients.end())
	{
		Client* disconnectedClient = m_clients[client_sock];

		std::string repCode = std::to_string(MC_DISCONNECT) + disconnectedClient->getUsername();

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
		notifyAllClients(repCode, client_sock, false);


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
		newAction.index = action.substr(5 + newAction.size, 5);
		newAction.newLineCount = action.substr(10 + newAction.size, 5);
		newAction.size += std::stoi(newAction.newLineCount);
		break;

	case MC_DELETE_REQUEST:
		newAction.dataLength = action.substr(0, 5);
		indexString = action.substr(5, 5);

		newAction.size = std::stoi(newAction.dataLength);
		newAction.index = indexString;
		newAction.newLineCount = action.substr(10, 5);
		break;

	case MC_REPLACE_REQUEST:
		newAction.selectionLength = action.substr(0, 5);
		newAction.dataLength = action.substr(5, 5);
		newAction.size = std::stoi(newAction.dataLength);
		newAction.data = action.substr(10, newAction.size);
		indexString = action.substr(10 + newAction.size, 5);
		newAction.index = indexString;
		newAction.newLineCount = action.substr(15 + newAction.size, 5);
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
	case MC_GET_USERS_ON_FILE_REQUEST:
		newAction.data = action;
		break;
	case MC_GET_USERS_REQUEST:
		newAction.data = action;
	case MC_GET_USERS_PERMISSIONS_REQ_REQUEST:
		newAction.data = action;
		break;
	case MC_POST_MSG_REQUEST:
		newAction.fileNameLength = action.substr(0, 5);
		newAction.size = std::stoi(newAction.fileNameLength);
		newAction.fileName = action.substr(5, newAction.size);
		newAction.dataLength = action.substr(5 + newAction.size, 5);
		newAction.data = action.substr(10 + newAction.size, std::stoi(newAction.dataLength));
		break;
	case MC_APPROVE_PERMISSION_REQUEST:
		newAction.fileNameLength = action.substr(0, 5);
		newAction.size = std::stoi(newAction.fileNameLength);
		newAction.fileName = action.substr(5, newAction.size);
		newAction.userNameLength = std::stoi(action.substr(5 + newAction.size, 5));
		newAction.userName = action.substr(10 + newAction.size, newAction.userNameLength);
		break;
	case MC_REJECT_PERMISSION_REQUEST:
		newAction.fileNameLength = action.substr(0, 5);
		newAction.size = std::stoi(newAction.fileNameLength);
		newAction.fileName = action.substr(5, newAction.size);
		newAction.userNameLength = std::stoi(action.substr(5 + newAction.size, 5));
		newAction.userName = action.substr(10 + newAction.size, newAction.userNameLength);
		break;
	case MC_PERMISSION_FILE_REQ_REQUEST:
		newAction.fileNameLength = action.substr(0, 5);
		newAction.size = std::stoi(newAction.fileNameLength);
		newAction.fileName = action.substr(5, newAction.size);
		newAction.userNameLength = std::stoi(action.substr(5 + newAction.size, 5));
		newAction.userName = action.substr(10 + newAction.size, newAction.userNameLength);
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

void Communicator::updateFileOnServerOld(const std::string& filePath, const Action& reqDetail)
{
	std::fstream file(filePath, std::ios::in | std::ios::out);
	if (!file.is_open()) {
		throw std::runtime_error("Failed to open file for reading/writing: " + filePath);
	}
	else {
		switch (reqDetail.code) {
		case MC_INSERT_REQUEST:
			// Insert operation
			operationHandler.insertOld(file, reqDetail.data, (std::stoi(reqDetail.index) + std::stoi(reqDetail.newLineCount)));
			break;

		case MC_DELETE_REQUEST:
			// Delete operation
			operationHandler.deleteContentOld(file, std::stoi(reqDetail.dataLength), (std::stoi(reqDetail.index) + std::stoi(reqDetail.newLineCount)),
				reqDetail.fileName);
			break;

		case MC_REPLACE_REQUEST:
			// Replace operation
			operationHandler.replaceOld(file, std::stoi(reqDetail.selectionLength), reqDetail.data,
				(std::stoi(reqDetail.index) + std::stoi(reqDetail.newLineCount)), reqDetail.fileName);
			break;

		default:
			throw std::runtime_error("Unknown action code: " + reqDetail.code);
		}

		file.close();
	}
}

void Communicator::updateFileOnServer(const std::string& filePath, const Action& reqDetail)
{
	auto it = m_filesData.find(filePath);

	if (it != m_filesData.end()) {
		int index;
		int length;
		switch (reqDetail.code) {
		case MC_INSERT_REQUEST:
			// Insert operation
			if (std::stoi(reqDetail.index) <= it->second.size()) {
				it->second.insert(std::stoi(reqDetail.index), reqDetail.data);
			}
			else {
				throw std::runtime_error("Insert index out of range");
			}
			break;

		case MC_DELETE_REQUEST:
			// Delete operation
			if (std::stoi(reqDetail.index) < it->second.size()) {
				it->second.erase(std::stoi(reqDetail.index), reqDetail.size);
			}
			else {
				throw std::runtime_error("Delete index out of range");
			}
			break;

		case MC_REPLACE_REQUEST:
			// Replace operation
			index = std::stoi(reqDetail.index);
			length = std::stoi(reqDetail.selectionLength);
			if (index < it->second.size() &&
				index + length <= it->second.size()) {
				it->second.erase(index, length);
				it->second.insert(index, reqDetail.data);
			}
			else {
				throw std::runtime_error("Replace index or selection length out of range");
			}
			break;

		default:
			throw std::runtime_error("Unknown action code: " + reqDetail.code);
		}
	}
	else {
		throw std::runtime_error("File not found: " + filePath);
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