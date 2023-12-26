#pragma once
#include "sqlite3.h"
#include <vector>
#include <iostream>
class DBHelper {
private:
    sqlite3* db;

public:
    DBHelper(std::string name) {
        const char* dbName = name.c_str();


        if (sqlite3_open(dbName, &db) != SQLITE_OK) {
            std::cerr << "Cannot open database\n";
            sqlite3_close(db);
            exit(1);
        }


        // Create a table
        if (sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS files (id INTEGER PRIMARY KEY, name TEXT, tablename TEXT);", 0, 0, 0) != SQLITE_OK) {
            std::cerr << "Table creation failed\n";
            sqlite3_close(db);
            exit(1);
        }

        std::cout << "Database opened successfully" << std::endl;
    }

    ~DBHelper() {
        sqlite3_close(db);
        std::cout << "Database closed successfully" << std::endl;
    }

    std::string getBlobTable(std::string filename)
    {
        // Execute a SELECT query and print the results
        std::string scom = "SELECT tablename FROM files WHERE name = " + filename + ";";
        const char* selectQuery = scom.c_str();
        sqlite3_stmt* statement;

        if (sqlite3_prepare_v2(db, selectQuery, -1, &statement, 0) == SQLITE_OK) {
            while (sqlite3_step(statement) == SQLITE_ROW) {
                const char* tableName = reinterpret_cast<const char*>(sqlite3_column_text(statement, 2));
                std::string ret(tableName);
                return ret;
            }
            sqlite3_finalize(statement);
        }
        else {
            std::cerr << "Query execution failed\n";
        }
    }
};