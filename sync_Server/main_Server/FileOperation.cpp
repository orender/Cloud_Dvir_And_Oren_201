#include "FileOperation.h"

std::string FileOperation::readFromFile(const std::string& filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open file: " + filePath);
    }

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    return content;
}

bool FileOperation::fileExists(const std::string& fileName)
{
    std::ifstream file(fileName);
    return file.good();
}

void FileOperation::createFile(const std::string& fileName, bool fileType)
{
    std::ofstream file(fileName);
    if (file.is_open())
    {
        if (fileType)
        {
            file << "Initial content of the file.";

        }
        file.close();
    }
    else
    {
        std::cerr << "Error creating the file: " << fileName << std::endl;
    }
}

void FileOperation::getFilesInDirectory(const std::string& directoryPath, std::vector<std::string>& files) {
    try {
        for (const auto& entry : std::filesystem::directory_iterator(directoryPath)) {
            if (std::filesystem::is_regular_file(entry)) {
                std::string fileName = entry.path().filename().string();

                if (std::find(files.begin(), files.end(), fileName) == files.end()) {
                    files.push_back(fileName);
                }
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error reading directory: " << e.what() << std::endl;
    }
}