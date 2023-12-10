#pragma once
#include <vector>
#define BUFFER std::vector<char>

#include "WSAInitializer.h"
#include <string>

enum MessageCodes
{
    MC_INITIAL_REQUEST = 101, //requests
    MC_INSERT_REQUEST = 102,
    MC_DELETE_REQUEST = 103,
    MC_REPLACE_REQUEST = 104,
    MC_CREATE_FILE_REQUEST = 105,
    MC_GET_FILES_REQUEST = 106,
    MC_CLOSE_FILE_REQUEST = 108,
    MC_ERR_RESP = 200, //responses
    MC_INITIAL_RESP = 201,
    MC_INSERT_RESP = 202,
    MC_DELETE_RESP = 203,
    MC_REPLACE_RESP = 204,
    MC_CREATE_FILE_RESP = 205,
    MC_GET_FILES_RESP = 206,
    MC_CLOSE_FILE_RESP = 208,
    MC_DISCONNECT = 300, //user
    MC_CLIENT_ID = 301
};

class Helper
{
public:
    static void sendData(const SOCKET sc, const BUFFER message);

    static BUFFER getPartFromSocket(const SOCKET sc, const int bytesNum);
    static BUFFER getPartFromSocket(const SOCKET sc, const int bytesNum, const int flags);
};
