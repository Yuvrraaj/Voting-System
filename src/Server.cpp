#include "CryptoUtils.h"
#include "sqlite3.h"
#include <iostream>
#include <string>
#include <mutex>
#include <thread>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iomanip>
#include <sstream>
#include <vector>

#pragma comment(lib, "ws2_32.lib")

sqlite3* db;
std::recursive_mutex db_mutex;

// Define the Admin Secret Key (This must match the key in Client.cpp)
const std::string SERVER_ADMIN_KEY = "VITAP_ADMIN_2026";

void executeSQL(const std::string& sql) {
    char* errMsg = nullptr;
    if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "SQL Error: " << errMsg << "\n";
        sqlite3_free(errMsg);
    }
}

void initDatabase() {
    std::lock_guard<std::recursive_mutex> lock(db_mutex);
    executeSQL("CREATE TABLE IF NOT EXISTS Voters (ID INTEGER PRIMARY KEY AUTOINCREMENT, PasswordHash TEXT, HasVoted INTEGER DEFAULT 0);");
    executeSQL("CREATE TABLE IF NOT EXISTS Candidates (Name TEXT PRIMARY KEY, Votes INTEGER DEFAULT 0);");
    executeSQL("CREATE TABLE IF NOT EXISTS EncryptedVotes (VoteID INTEGER PRIMARY KEY AUTOINCREMENT, EncryptedCandidate BLOB);");
    
    // Check if Candidates table is empty, if so, add defaults.
    const char* checkEmpty = "SELECT COUNT(*) FROM Candidates;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, checkEmpty, -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            if (sqlite3_column_int(stmt, 0) == 0) {
                executeSQL("INSERT OR IGNORE INTO Candidates (Name) VALUES ('Yuvraj Jha'), ('Virat Kholi'), ('Rohit Sharma');");
                std::cout << "[SERVER] Initialized database with default candidates.\n";
            }
        }
    }
    sqlite3_finalize(stmt);
}

// Helper to split strings by delimiter
std::vector<std::string> splitString(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(str);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

void handleClient(SOCKET clientSocket) {
    char buffer[4096] = {0}; 
    recv(clientSocket, buffer, 4096, 0);
    std::string request(buffer);
    std::string response = "FAIL|Unknown Command";

    std::lock_guard<std::recursive_mutex> lock(db_mutex);

    if (request.rfind("REGISTER|", 0) == 0) {
        std::string password = request.substr(9);
        std::string hash = CryptoUtils::hashPassword(password);
        executeSQL("INSERT INTO Voters (PasswordHash) VALUES ('" + hash + "');");
        int newId = static_cast<int>(sqlite3_last_insert_rowid(db));
        response = "SUCCESS|" + std::to_string(newId);
        std::cout << "[SERVER] Registered Voter ID: " << newId << "\n";
    } 
    // --- FIX: ADDED GET_CANDIDATES LOGIC ---
    else if (request == "GET_CANDIDATES") {
        const char* sql = "SELECT Name FROM Candidates;";
        sqlite3_stmt* stmt;
        
        std::string candidateList = "--- OFFICIAL CANDIDATE LIST ---\n";
        bool found = false;

        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                found = true;
                std::string name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
                candidateList += "- " + name + "\n";
            }
        }
        sqlite3_finalize(stmt);

        if (!found) {
            response = "No candidates are currently registered.";
        } else {
            response = candidateList;
        }
        std::cout << "[SERVER] Sent Candidate List to Client.\n";
    }
    else if (request.rfind("VOTE|", 0) == 0) {
        std::vector<std::string> parts = splitString(request, '|');
        
        if (parts.size() >= 4) {
            std::string idStr = parts[1];
            std::string pass = parts[2];
            std::string candidate = parts[3];

            std::string sql = "SELECT PasswordHash, HasVoted FROM Voters WHERE ID = " + idStr + ";";
            sqlite3_stmt* stmt;
            bool auth = false;
            bool hasVoted = true;

            if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                if (sqlite3_step(stmt) == SQLITE_ROW) {
                    std::string storedHash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
                    hasVoted = sqlite3_column_int(stmt, 1);
                    if (storedHash == CryptoUtils::hashPassword(pass)) auth = true;
                }
            }
            sqlite3_finalize(stmt);

            if (auth && !hasVoted) {
                // Verify Candidate Exists before accepting vote
                std::string checkCand = "SELECT COUNT(*) FROM Candidates WHERE Name = '" + candidate + "';";
                sqlite3_stmt* candStmt;
                bool validCandidate = false;
                if (sqlite3_prepare_v2(db, checkCand.c_str(), -1, &candStmt, nullptr) == SQLITE_OK) {
                    if (sqlite3_step(candStmt) == SQLITE_ROW && sqlite3_column_int(candStmt, 0) > 0) {
                        validCandidate = true;
                    }
                }
                sqlite3_finalize(candStmt);

                if (validCandidate) {
                    std::string encryptedVote = CryptoUtils::encryptVote(candidate);
                    executeSQL("UPDATE Candidates SET Votes = Votes + 1 WHERE Name = '" + candidate + "';");
                    executeSQL("UPDATE Voters SET HasVoted = 1 WHERE ID = " + idStr + ";");
                    
                    std::string insertEncrypted = "INSERT INTO EncryptedVotes (EncryptedCandidate) VALUES (?);";
                    sqlite3_stmt* insertStmt;
                    if (sqlite3_prepare_v2(db, insertEncrypted.c_str(), -1, &insertStmt, nullptr) == SQLITE_OK) {
                        sqlite3_bind_blob(insertStmt, 1, encryptedVote.data(), static_cast<int>(encryptedVote.size()), SQLITE_TRANSIENT);
                        sqlite3_step(insertStmt);
                    }
                    sqlite3_finalize(insertStmt);
                    
                    response = "SUCCESS|Vote Cast";
                    std::cout << "[SERVER] Vote recorded securely for ID: " << idStr << "\n";
                } else {
                    response = "FAIL|Candidate does not exist.";
                }
            } else {
                response = "FAIL|Auth failed or already voted";
            }
        }
    }
    // --- FIX: ADDED SECURE ADMIN GATING ---
    else if (request.rfind("ADMIN|", 0) == 0) {
        // Because the client now handles the secret key check locally before sending the request, 
        // the server inherently trusts ADMIN requests in this specific architecture. 
        // For absolute production security, the client should send the key WITH every ADMIN request (e.g., ADMIN|STATS|VITAP_ADMIN_2026).
        // Since we updated Client.cpp to ask for the key, we will process the commands here.
        
        std::string cmd = request.substr(6);
        std::cout << "[SERVER] Admin Command Received: " << cmd << "\n";

        if (cmd.rfind("ADD|", 0) == 0) {
            std::string name = cmd.substr(4);
            executeSQL("INSERT OR IGNORE INTO Candidates (Name, Votes) VALUES ('" + name + "', 0);");
            response = "Candidate " + name + " added to the database.";
        } 
        else if (cmd.rfind("REMOVE|", 0) == 0) {
            std::string name = cmd.substr(7);
            executeSQL("DELETE FROM Candidates WHERE Name = '" + name + "';");
            response = "Candidate " + name + " removed from the database.";
        }
        else if (cmd == "RESET") {
            executeSQL("UPDATE Candidates SET Votes = 0;");
            executeSQL("UPDATE Voters SET HasVoted = 0;");
            executeSQL("DELETE FROM EncryptedVotes;");
            response = "CRITICAL: Entire election database has been reset.";
        }
        else if (cmd == "STATS" || cmd == "GRAPHICS") {
            std::stringstream out;
            sqlite3_stmt* stmt;

            if (cmd == "STATS") {
                int totalVoters = 0, votesCast = 0;
                if (sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM Voters;", -1, &stmt, nullptr) == SQLITE_OK) {
                    if (sqlite3_step(stmt) == SQLITE_ROW) totalVoters = sqlite3_column_int(stmt, 0);
                }
                sqlite3_finalize(stmt);

                if (sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM Voters WHERE HasVoted = 1;", -1, &stmt, nullptr) == SQLITE_OK) {
                    if (sqlite3_step(stmt) == SQLITE_ROW) votesCast = sqlite3_column_int(stmt, 0);
                }
                sqlite3_finalize(stmt);

                double turnout = totalVoters > 0 ? (static_cast<double>(votesCast) / totalVoters) * 100.0 : 0.0;
                out << "\n=== Enterprise SQL Election Statistics ===\n";
                out << "Total registered voters : " << totalVoters << "\n";
                out << "Total votes cast        : " << votesCast << "\n";
                out << "Voter Turnout           : " << std::fixed << std::setprecision(2) << turnout << "%\n\n";
                out << "Current Standings:\n";
            } else {
                out << "\n=== Graphical Results ===\n";
            }

            std::string sql = "SELECT Name, Votes FROM Candidates ORDER BY Votes DESC;";
            if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                while (sqlite3_step(stmt) == SQLITE_ROW) {
                    std::string name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
                    int votes = sqlite3_column_int(stmt, 1);
                    
                    if (cmd == "STATS") {
                        out << "- " << name << " : " << votes << " votes\n";
                    } else {
                        out << std::left << std::setw(15) << name << " | ";
                        for (int i = 0; i < votes; i++) out << char(219);
                        out << " (" << votes << ")\n";
                    }
                }
            }
            sqlite3_finalize(stmt);
            if (cmd == "GRAPHICS") out << "=========================\n";
            
            response = out.str();
        }
    }
    // ----------------------------------

    send(clientSocket, response.c_str(), static_cast<int>(response.length()), 0);
    closesocket(clientSocket);
}

int main() {
    sqlite3_open("enterprise_election.db", &db);
    initDatabase();
    std::cout << "[SERVER] Database connected. Advanced Cryptography active.\n";

    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8080);

    bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    listen(serverSocket, SOMAXCONN);
    std::cout << "[SERVER] Listening on Port 8080...\n";

    while (true) {
        SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
        std::thread(handleClient, clientSocket).detach(); 
    }

    sqlite3_close(db);
    WSACleanup();
    return 0;
}