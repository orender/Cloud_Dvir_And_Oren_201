#pragma once
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>

class Operations
{
public:
    void insert(std::string& fileData, const std::string& data, const int& index);
    void deleteContent(std::string& fileData, const int& lengthToDelete, const int& index);
    void replace(std::string& fileData, const int& selectionLength, const std::string& replacementText, const int& index);

    void insertOld(std::fstream& file, const std::string& data, const int& index);
    void deleteContentOld(std::fstream& file, const int& lengthToDelete, const int& index, std::string name);
    void replaceOld(std::fstream& file, const int& selectionLength, const std::string& replacementText, const int& index, std::string name);

};

