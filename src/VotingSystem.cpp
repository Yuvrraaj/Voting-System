#include "VotingSystem.h"
#include "CryptoUtils.h"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <ctime>

VotingSystem::VotingSystem() {
    if (sqlite3_open("election_data.db", &db) != SQLITE_OK) {
        std::cerr << "Failed to open database: " << sqlite3_errmsg(db) << "\n";
    } else {
        initializeDatabase();
        logEvent("System initialized securely. SQLite Database connected.");
    }
}

VotingSystem::~VotingSystem() {
    sqlite3_close(db);
}

void VotingSystem::executeSQL(const std::string& sql) const {
    char* errMsg = nullptr;
    if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "SQL Error: " << errMsg << "\n";
        sqlite3_free(errMsg);
    }
}

void VotingSystem::initializeDatabase() {
    std::lock_guard<std::recursive_mutex> lock(db_mutex);

    std::string createVotersTable = 
        "CREATE TABLE IF NOT EXISTS Voters ("
        "ID INTEGER PRIMARY KEY AUTOINCREMENT, "
        "PasswordHash TEXT NOT NULL, "
        "HasVoted INTEGER DEFAULT 0, "
        "FailedAttempts INTEGER DEFAULT 0, "
        "IsLocked INTEGER DEFAULT 0);";

    std::string createCandidatesTable = 
        "CREATE TABLE IF NOT EXISTS Candidates ("
        "Name TEXT PRIMARY KEY, "
        "Votes INTEGER DEFAULT 0);";

    std::string createLogsTable = 
        "CREATE TABLE IF NOT EXISTS AuditLogs ("
        "Timestamp TEXT, "
        "Event TEXT);";
        
    std::string createEncryptedVotesTable = 
        "CREATE TABLE IF NOT EXISTS EncryptedVotes ("
        "VoteID INTEGER PRIMARY KEY AUTOINCREMENT, "
        "EncryptedCandidate TEXT);";

    executeSQL(createVotersTable);
    executeSQL(createCandidatesTable);
    executeSQL(createLogsTable);
    executeSQL(createEncryptedVotesTable);

    std::string seedCandidates = 
        "INSERT OR IGNORE INTO Candidates (Name) VALUES ('Alice Smith'), ('Bob Jones'), ('Charlie Brown');";
    executeSQL(seedCandidates);
}

void VotingSystem::logEvent(const std::string& event) const {
    std::lock_guard<std::recursive_mutex> lock(db_mutex);
    std::string sql = "INSERT INTO AuditLogs (Timestamp, Event) VALUES ('" + 
                      CryptoUtils::getCurrentTimestamp() + "', '" + event + "');";
    executeSQL(sql);
}

bool VotingSystem::isVotingOpen() const {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    struct tm timeinfo;
    #ifdef _WIN32
        localtime_s(&timeinfo, &now_c);
    #else
        localtime_r(&now_c, &timeinfo);
    #endif
    return (timeinfo.tm_hour >= 0 && timeinfo.tm_hour <= 23); 
}

int VotingSystem::registerVoter(const std::string& password) {
    std::lock_guard<std::recursive_mutex> lock(db_mutex); 
    
    std::string hash = CryptoUtils::hashPassword(password);
    std::string sql = "INSERT INTO Voters (PasswordHash) VALUES ('" + hash + "');";
    executeSQL(sql);

    int newId = static_cast<int>(sqlite3_last_insert_rowid(db));
    logEvent("Voter registered with System ID: " + std::to_string(newId));
    return newId;
}

bool VotingSystem::authenticateVoter(int voterId, const std::string& password) {
    std::lock_guard<std::recursive_mutex> lock(db_mutex);

    std::string sql = "SELECT PasswordHash, IsLocked FROM Voters WHERE ID = " + std::to_string(voterId) + ";";
    sqlite3_stmt* stmt;
    bool authSuccess = false;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            std::string storedHash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            int isLocked = sqlite3_column_int(stmt, 1);

            if (isLocked == 1) {
                std::cout << "Account locked for security. Too many failed attempts.\n";
                logEvent("Login blocked: Account " + std::to_string(voterId) + " is locked.");
            } else if (storedHash == CryptoUtils::hashPassword(password)) {
                executeSQL("UPDATE Voters SET FailedAttempts = 0 WHERE ID = " + std::to_string(voterId) + ";");
                authSuccess = true;
                logEvent("Voter " + std::to_string(voterId) + " successfully logged in.");
            } else {
                executeSQL("UPDATE Voters SET FailedAttempts = FailedAttempts + 1 WHERE ID = " + std::to_string(voterId) + ";");
                executeSQL("UPDATE Voters SET IsLocked = 1 WHERE ID = " + std::to_string(voterId) + " AND FailedAttempts >= 3;");
                logEvent("Failed login attempt for ID: " + std::to_string(voterId));
            }
        } else {
            std::cout << "Error: Voter ID not found.\n";
        }
    }
    sqlite3_finalize(stmt);
    return authSuccess;
}

bool VotingSystem::castVote(int voterId, const std::string& candidateName) {
    if (!isVotingOpen()) {
        std::cout << "Error: Voting is currently closed.\n";
        return false;
    }

    std::lock_guard<std::recursive_mutex> lock(db_mutex); 

    std::string checkVoteSql = "SELECT HasVoted FROM Voters WHERE ID = " + std::to_string(voterId) + ";";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, checkVoteSql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW && sqlite3_column_int(stmt, 0) == 1) {
            std::cout << "Error: You have already cast your vote.\n";
            sqlite3_finalize(stmt);
            return false;
        }
    }
    sqlite3_finalize(stmt);

    std::string checkCandidateSql = "SELECT Name FROM Candidates WHERE Name = '" + candidateName + "';";
    bool validCandidate = false;
    if (sqlite3_prepare_v2(db, checkCandidateSql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            validCandidate = true;
        }
    }
    sqlite3_finalize(stmt);

    if (!validCandidate) {
        std::cout << "Error: Invalid candidate.\n";
        return false;
    }

    std::string encryptedVote = CryptoUtils::encryptVote(candidateName);
    executeSQL("UPDATE Candidates SET Votes = Votes + 1 WHERE Name = '" + candidateName + "';");
    executeSQL("UPDATE Voters SET HasVoted = 1 WHERE ID = " + std::to_string(voterId) + ";");
    
    // --- NEW PREPARED STATEMENT FIX ---
    std::string insertEncrypted = "INSERT INTO EncryptedVotes (EncryptedCandidate) VALUES (?);";
    sqlite3_stmt* insertStmt;
    if (sqlite3_prepare_v2(db, insertEncrypted.c_str(), -1, &insertStmt, nullptr) == SQLITE_OK) {
        // Bind the encrypted string safely to the '?' placeholder
        sqlite3_bind_text(insertStmt, 1, encryptedVote.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(insertStmt);
    }
    sqlite3_finalize(insertStmt);
    // ----------------------------------
    logEvent("Encrypted vote cast securely by Voter " + std::to_string(voterId));
    std::cout << "Vote securely encrypted and recorded in SQL Database!\n";
    return true;
}

void VotingSystem::addCandidate(const std::string& name) {
    std::lock_guard<std::recursive_mutex> lock(db_mutex);
    std::string sql = "INSERT OR IGNORE INTO Candidates (Name, Votes) VALUES ('" + name + "', 0);";
    executeSQL(sql);
    logEvent("Admin added candidate: " + name);
    std::cout << "Candidate added successfully.\n";
}

void VotingSystem::removeCandidate(const std::string& name) {
    std::lock_guard<std::recursive_mutex> lock(db_mutex);
    std::string sql = "DELETE FROM Candidates WHERE Name = '" + name + "';";
    executeSQL(sql);
    logEvent("Admin removed candidate: " + name);
    std::cout << "Candidate removed successfully.\n";
}

void VotingSystem::displayCandidates() const {
    std::lock_guard<std::recursive_mutex> lock(db_mutex);
    std::string sql = "SELECT Name FROM Candidates;";
    sqlite3_stmt* stmt;

    std::cout << "\n--- Available Candidates ---\n";
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            std::cout << "- " << reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)) << "\n";
        }
    }
    sqlite3_finalize(stmt);
}

void VotingSystem::showElectionStatistics() const {
    std::lock_guard<std::recursive_mutex> lock(db_mutex);
    sqlite3_stmt* stmt;

    int totalVoters = 0;
    if (sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM Voters;", -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) totalVoters = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);

    int votesCast = 0;
    if (sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM Voters WHERE HasVoted = 1;", -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) votesCast = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);

    double turnout = totalVoters > 0 ? (static_cast<double>(votesCast) / totalVoters) * 100.0 : 0.0;

    std::cout << "\n=== Enterprise SQL Election Statistics ===\n";
    std::cout << "Total registered voters : " << totalVoters << "\n";
    std::cout << "Total votes cast        : " << votesCast << "\n";
    std::cout << "Voter Turnout           : " << std::fixed << std::setprecision(2) << turnout << "%\n\n";

    std::cout << "Current Standings:\n";
    std::string sql = "SELECT Name, Votes FROM Candidates ORDER BY Votes DESC;";
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            std::string name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            int votes = sqlite3_column_int(stmt, 1);
            std::cout << "- " << name << " : " << votes << " votes\n";
        }
    }
    sqlite3_finalize(stmt);
    logEvent("Admin viewed full election statistics.");
}

void VotingSystem::generateGraphicalVisualization() const {
    std::lock_guard<std::recursive_mutex> lock(db_mutex);
    std::string sql = "SELECT Name, Votes FROM Candidates ORDER BY Votes DESC;";
    sqlite3_stmt* stmt;

    std::cout << "\n=== Graphical Results ===\n";
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            std::string name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            int votes = sqlite3_column_int(stmt, 1);
            
            std::cout << std::left << std::setw(15) << name << " | ";
            for (int i = 0; i < votes; i++) {
                std::cout << char(219); 
            }
            std::cout << " (" << votes << ")\n";
        }
    }
    sqlite3_finalize(stmt);
    std::cout << "=========================\n";
    logEvent("Admin generated graphical visualization.");
}

void VotingSystem::resetElection() {
    std::lock_guard<std::recursive_mutex> lock(db_mutex);
    executeSQL("UPDATE Candidates SET Votes = 0;");
    executeSQL("UPDATE Voters SET HasVoted = 0;");
    executeSQL("DELETE FROM EncryptedVotes;");
    logEvent("CRITICAL: Admin performed a complete database reset.");
    std::cout << "\nSUCCESS: Election has been completely reset. All votes cleared.\n";
}