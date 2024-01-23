#include "dataSplitter.h"

std::vector<std::string> splitString(const std::string& input, size_t sliceSize) {
    std::vector<std::string> result;

    // Calculate the total number of slices
    size_t totalSlices = (input.length() + sliceSize - 1) / sliceSize;

    // Start index for the current slice
    size_t startIndex = 0;

    for (size_t i = 0; i < totalSlices; ++i) {
        // Extract the substring for the current slice
        result.push_back(input.substr(startIndex, sliceSize));

        // Move the start index to the next slice
        startIndex += sliceSize;
    }

    return result;
}

dataSplitter::dataSplitter(std::string db_name)
{
    std::string sc1 = "c1";
    std::string sc2 = "c2";
    std::string sc3 = "c3";
    std::string sc4 = "c4";

    unsigned int         PortContainer1 = 12345;
    unsigned int         PortContainer2 = 12350;
    unsigned int         PortContainer3 = 12355;
    unsigned int         PortContainer4 = 12360;

    container* c1 = new container("127.0.0.1", PortContainer1);
    container* c2 = new container("127.0.0.1", PortContainer2);
    container* c3 = new container("127.0.0.1", PortContainer3);
    container* c4 = new container("127.0.0.1", PortContainer4);

    //connect to containers
    containers[sc1] = *c1;
    containers[sc2] = *c2;
    containers[sc3] = *c3;
    containers[sc4] = *c4;

    std::cout << c1->start() << std::endl;
    std::cout << c2->start() << std::endl;
    std::cout << c3->start() << std::endl;
    std::cout << c4->start() << std::endl;
    



    const char* dbName = db_name.c_str();


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

std::string createSixDigitString(int first, int second) {
    std::ostringstream oss;
    oss << std::setw(3) << std::setfill('0') << first
        << std::setw(3) << std::setfill('0') << second;
    return oss.str();
}

bool dataSplitter::saveNewFile(std::string file_name, std::string file_data)
{

    // Insert some data, current table naming is temporary
    std::string sqlcom = "INSERT INTO  files (name, tablename) VALUES('" + file_name + "', '" + file_name + "');";
    if (sqlite3_exec(db, sqlcom.c_str(), 0, 0, 0) != SQLITE_OK) {
        std::cerr << "Data insertion failed\n";
        return false;
    }
    int lastInsertedId = sqlite3_last_insert_rowid(db);
    int table_id = lastInsertedId;

    sqlcom = "CREATE TABLE IF NOT EXISTS " + file_name + " (id INTEGER PRIMARY KEY, container TEXT, bloborder INTEGER);";
    // Create a table
    if (sqlite3_exec(db, sqlcom.c_str(), 0, 0, 0) != SQLITE_OK) {
        std::cerr << "Table creation failed: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    std::vector<std::string> pieces = splitString(file_data, blobSize);
    std::map<std::string, container>::iterator iter = containers.begin();
    std::string container1 = "";
    std::string container2 = "";
    for (int i = 0; i < pieces.size(); i++)
    {
        if (iter == containers.end())
        {
            iter = containers.begin();
        }

        container1 = iter->first;

        iter++;

        if (iter == containers.end())
        {
            iter = containers.begin();
        }

        container2 = iter->first;

        // Insert some data
        sqlcom = "INSERT INTO " + file_name + " (container, bloborder) VALUES('" + container1 + "b" + container2 + "', " + std::to_string(i + 1) + ");";
        if (sqlite3_exec(db, sqlcom.c_str(), 0, 0, 0) != SQLITE_OK) {
            std::cerr << "Table creation failed: " << sqlite3_errmsg(db) << std::endl;
            return false;
        }

        lastInsertedId = sqlite3_last_insert_rowid(db);

        containers[container1].save(pieces[i], createSixDigitString(table_id, lastInsertedId));
        containers[container2].save(pieces[i], createSixDigitString(table_id, lastInsertedId));

    }

    return true;
}

void splitString(const std::string& input, std::string& first, std::string& second, char delimiter = 'b') {
    std::istringstream stream(input);

    std::getline(stream, first, delimiter);
    std::getline(stream, second, delimiter);
}

std::string dataSplitter::getFileData(std::string file_name)
{
    std::string file_data = "";

    // Execute a SELECT query and print the results
    std::string scom = "SELECT id, tablename FROM files WHERE name = '" + file_name + "';";
    const char* selectQuery = scom.c_str();
    sqlite3_stmt* statement;
    std::string table_name = "";
    int id = -1;

    if (sqlite3_prepare_v2(db, selectQuery, -1, &statement, 0) == SQLITE_OK) {
        while (sqlite3_step(statement) == SQLITE_ROW) {
            id = sqlite3_column_int(statement, 0);
            const char* tableName = reinterpret_cast<const char*>(sqlite3_column_text(statement, 1));
            std::string ret(tableName);
            table_name = ret;
        }
        sqlite3_finalize(statement);
    }
    else {
        std::cerr << "Query execution failed\n";
    }
    std::string temp = "";
    scom = "SELECT * FROM " + table_name + ";";
    const char* selectQueryBlobs = scom.c_str();
    sqlite3_stmt* statementBlobs;
    int blobId = -1;
    std::string c1, c2;
    if (sqlite3_prepare_v2(db, selectQueryBlobs, -1, &statementBlobs, 0) == SQLITE_OK) {
        while (sqlite3_step(statementBlobs) == SQLITE_ROW) {
            blobId = sqlite3_column_int(statementBlobs, 0);
            const char* container = reinterpret_cast<const char*>(sqlite3_column_text(statementBlobs, 1));
            std::string containers_names(container);
            splitString(containers_names, c1, c2);
            temp = containers[c1].getBlob(createSixDigitString(id, blobId));
            if (temp == failGet)
            {
                temp = containers[c2].getBlob(createSixDigitString(id, blobId));
                if (temp == failGet)
                {
                    return failGet;
                }
                file_data += temp;
            }
            else {
                file_data += temp;
            }
        }
        //sqlite3_finalize(statement);
        return file_data;
    }
    else {
        std::cerr << "Query execution failed\n";
    }
    return std::string();
}

bool dataSplitter::deleteFile(std::string file_name)
{
    std::string file_data = "";

    // Execute a SELECT query and print the results
    std::string scom = "SELECT id, tablename FROM files WHERE name = '" + file_name + "';";
    const char* selectQuery = scom.c_str();
    sqlite3_stmt* statement;
    std::string table_name = "";
    int id = -1;

    if (sqlite3_prepare_v2(db, selectQuery, -1, &statement, 0) == SQLITE_OK) {
        while (sqlite3_step(statement) == SQLITE_ROW) {
            id = sqlite3_column_int(statement, 0);
            const char* tableName = reinterpret_cast<const char*>(sqlite3_column_text(statement, 1));
            std::string ret(tableName);
            table_name = ret;
        }
        sqlite3_finalize(statement);
    }
    else {
        std::cerr << "Query execution failed\n";
    }
    std::string temp = "";
    scom = "SELECT * FROM " + table_name + ";";
    const char* selectQueryBlobs = scom.c_str();
    sqlite3_stmt* statementBlobs;
    int blobId = -1;
    std::string c1, c2;
    if (sqlite3_prepare_v2(db, selectQueryBlobs, -1, &statementBlobs, 0) == SQLITE_OK) {
        while (sqlite3_step(statementBlobs) == SQLITE_ROW) {
            blobId = sqlite3_column_int(statementBlobs, 0);
            const char* container = reinterpret_cast<const char*>(sqlite3_column_text(statementBlobs, 1));
            std::string containers_names(container);
            splitString(containers_names, c1, c2);
            temp = containers[c1].getBlob(createSixDigitString(id, blobId));
            std::cout << temp << std::endl;
            temp = containers[c2].deleteBlob(createSixDigitString(id, blobId));
            std::cout << temp << std::endl;
        }

        scom = "DELETE FROM files WHERE name = '" + file_name + "';";
        if (sqlite3_exec(db, scom.c_str(), 0, 0, 0) != SQLITE_OK)
        {
            return false;
        }

        scom = "DROP TABLE " + table_name + ";";
        if (sqlite3_exec(db, scom.c_str(), 0, 0, 0) != SQLITE_OK)
        {
            return false;
        }
        //sqlite3_finalize(statement);
        return true;
    }
    else {
        std::cerr << "Query execution failed\n";
    }


    return false;
}

std::string dataSplitter::getFiles()
{
    std::string files = "";

    // Execute a SELECT query and print the results
    std::string scom = "SELECT id, tablename FROM files;";
    const char* selectQuery = scom.c_str();
    sqlite3_stmt* statement;
    std::string table_name = "";
    int id = -1;

    if (sqlite3_prepare_v2(db, selectQuery, -1, &statement, 0) == SQLITE_OK) {
        while (sqlite3_step(statement) == SQLITE_ROW) {
            id = sqlite3_column_int(statement, 0);
            const char* tableName = reinterpret_cast<const char*>(sqlite3_column_text(statement, 1));
            std::string ret(tableName);
            files += ret + "\n";
        }
        sqlite3_finalize(statement);
    }
    else {
        std::cerr << "Query execution failed\n";
        return std::string();
    }
    return files;
}
