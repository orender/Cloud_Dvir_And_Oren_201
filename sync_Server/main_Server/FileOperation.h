#pragma once
#include <iostream>
#include <sstream>
#include <fstream>
#include <filesystem>

class FileOperation
{
public:
    bool fileExists(const std::string& fileName);
    void createFile(const std::string& fileName, bool fileType);
    void getFilesInDirectory(const std::string& directoryPath, std::vector<std::string>& files);
    std::string readFromFile(const std::string& filePath);
};

