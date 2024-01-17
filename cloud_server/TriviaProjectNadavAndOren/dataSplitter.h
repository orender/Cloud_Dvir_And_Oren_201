#pragma once
#include "container.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <map>
#define blobSize 30
#define failGet "oops i did it again"

class dataSplitter
{
private:
	std::map<std::string, container> containers;
	sqlite3* db;

public:
	dataSplitter(std::string db_name);
	bool saveNewFile(std::string file_name, std::string file_data);
	std::string getFileData(std::string file_name);
	//bool deleteFile(std::string file_name);
};