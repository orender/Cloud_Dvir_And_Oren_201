#pragma once
#include <iostream>
#include <fstream>
#include <sstream>

class Operations
{
public:
    void insert(std::fstream& file, const std::string& data, const int& index);
    void deleteContent(std::fstream& file, const int& lengthToDelete, const int& index, std::string name);
    void replace(std::fstream& file, const int& selectionLength, const std::string& replacementText, const int& index, std::string name);

};

