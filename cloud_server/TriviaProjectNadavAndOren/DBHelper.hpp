#pragma once
#include "sqlite3.h"
#include <iostream>
class DBHelper {
private:
    sqlite3* db;

public:
    DBHelper(std::string name) {
        const char* dbName = name.c_str();
        int rc = sqlite3_open(dbName, &db);

        if (rc) {
            std::cerr << "Cannot open database: " << sqlite3_errmsg(db) << std::endl;
            sqlite3_close(db);
            exit(1);
        }

        std::cout << "Database opened successfully" << std::endl;
    }

    ~DBHelper() {
        sqlite3_close(db);
        std::cout << "Database closed successfully" << std::endl;
    }

    void executeQuery(const char* query) {
        char* errMsg = nullptr;

        int rc = sqlite3_exec(db, query, nullptr, nullptr, &errMsg);

        if (rc != SQLITE_OK) {
            std::cerr << "SQL error: " << errMsg << std::endl;
            sqlite3_free(errMsg);
        }
        else {
            std::cout << "Query executed successfully" << std::endl;
        }
    }
};